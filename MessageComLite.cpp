/*
	MessageComLite.cpp
	
	MessageComLite is a library to facilitate communication between devices.
	MessageComLite uses the following Arduino libraries:
	
	SoftwareSerial library
	Link:
		http://arduino.cc/de/Reference/SoftwareSerial
	
	Base64 library by Adam Rudd
	Link:
		https://github.com/adamvr/arduino-base64

  The Idea of this library is based on the MessageCom library
  Link:
  	https://github.com/sigger/MessageCom

	@version 0.5
	@date 11 Nov 2013

	@author master[at]link-igor[dot]de Igor Milutinovic
	@author hellokitty_iva[at]gmx[dot]de Iva Milutinovic

	@link https://github.com/sigger/MessageComLite

	Copyright 2013 Igor Milutinovic, Iva Milutinovic. All rights reserved.

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
	
#include <MessageComLite.h>

// private
int MessageComLite::indexOf(uint8_t *array, uint8_t value, uint8_t startPos, uint8_t endPos) {
	if(endPos == 0)
		endPos = _maxSize;
	for(uint8_t pos=startPos; pos<=endPos; pos++) {
		
		if(array[pos]==value)
			return pos;
	}
	return -1;
}
uint8_t MessageComLite::extendDataTo(uint8_t bytes) {
	uint8_t retValue = 0;

	if(_dataSize == 0) {
		_dataSize = bytes;
		retValue = 1;
	} else {
		_dataSize += (bytes+1);
		retValue = 2;
	}

	if(_dataSize <= _maxSize)
		return retValue;
	return 0;
}
void MessageComLite::getPositionsOfIndexFromData(uint8_t index, uint8_t &len, int &start, int &stop) {
	len = 0, start = 0, stop = 0;

	for(uint8_t i=0; i<=index; i++) {
		if((stop = indexOf(_data, _delimiter, start, _dataSize)) < 0) {
			stop = _dataSize;
			break;
		}
		if(i==index)
			break;
		start = stop+1;
	}

	len = ((stop-start)+1);
}
boolean MessageComLite::bytePlausible(uint8_t value) {
	// _startDelimiter || _stopDelimiter || + || / || 0 to 9 || = || A to Z || a to z
	// in regular messages ackChar and nackChar is not plausible char
	// ackChar and nackChar is just used for acknowledgements
	if(value == _startDelimiter || value == _stopDelimiter ||
		value == 43 || value == 47 || (48 <= value && value <= 57) || value == 61 ||
		(65 <= value && value <= 90) || (97 <= value && value <= 122))
		return 1;
	return 0;
}
void MessageComLite::skipBytes(uint8_t bytes) {
	if(_Serial != NULL)
		while(bytes--)
			_Serial->read();
	else if(_swSerial != NULL)
		while(bytes--)
			_swSerial->read();
}



// public
MessageComLite::MessageComLite(HardwareSerial &hwSerial, uint8_t *buffer, uint8_t buffer_maxSize, uint8_t *msg, uint8_t maxSize) {
	// use the adress of the user defined array for the message
	_bufferMaxSize = buffer_maxSize;
	_buffer = buffer;

	_maxSize = maxSize;
	_msg = msg;

	_startDelimiter = '#';
	_stopDelimiter = ';';
	_delimiter = '|';
	_ackChar = '@';
	_nackChar = '!';
	
	_Serial = &hwSerial;
	_swSerial = NULL;

	clear();
}
MessageComLite::MessageComLite(SoftwareSerial &swSerial, uint8_t *buffer, uint8_t buffer_maxSize, uint8_t *msg, uint8_t maxSize) {
	// use the adress of the user defined array for the message
	_bufferMaxSize = buffer_maxSize;
	_buffer = buffer;

	_maxSize = maxSize;
	_msg = msg;

	_startDelimiter = '#';
	_stopDelimiter = ';';
	_delimiter = '|';
	_ackChar = '@';
	_nackChar = '!';

	_Serial = NULL;
	_swSerial = &swSerial;

	clear();
}

uint8_t MessageComLite::getSize() {
	return _size;
}

// clean up
void MessageComLite::clear() {
	// clear msg
	for(uint8_t i=0; i<_bufferMaxSize; i++)
		_buffer[i] = 0;

	for(uint8_t i=0; i<_maxSize; i++)
		_msg[i] = 0;

	_dataSize = 0;
	_bufferSize = 0;
	_size = 0;

	_version = 2;
	_type = 0;

	_messageNumber = 1;
	_totalQuantity = 1;

	_dataCount = 0;
	_nextData = 0; 

	_data = &_msg[6];

	_commandStatus = 0;
	_checksum = 0;
	_csH = 0;
	_csL = 0;

	if(_Serial != NULL)
		_Serial->flush();
	else if(_swSerial != NULL)
		_swSerial->flush();
}

// identification
void MessageComLite::getVersionFromMessage() {
	// read the _version from the message
	_version = _msg[0];
}
void MessageComLite::getTypeFromMessage() {
	// read the _type from the message
	_type = _msg[1];
}
void MessageComLite::setVersion(uint8_t version) {
	_version = version;
}
void MessageComLite::setType(uint8_t type) {
	_type = type;
}

// Command Status
void MessageComLite::getCommandStatusFromMessage() {
	// read the _commandStatus from the message
	_commandStatus = _msg[2];
}
void MessageComLite::setCommandStatus(boolean state, uint8_t taskValue) {
	_commandStatus = createCommandStatus(state, taskValue);
}
uint8_t MessageComLite::createCommandStatus(boolean state, uint8_t taskValue) {
	// check the size of the task value
	// value is greater than 7-bit -> set value to maximum: 127
	if(0 > taskValue && taskValue >= 128)
		taskValue = 127;
	// else: value consists of 7-bit -> everything is right

	// add state-flag (succeed-flag) to taskValue if set
	if(state)
		taskValue += 128;

	return taskValue;
}
uint8_t MessageComLite::getTaskValue() {
	return getTaskValueFromCommandStatus(_commandStatus);
}
uint8_t MessageComLite::getTaskValueFromCommandStatus(uint8_t cmdStatus) {
	bitClear(cmdStatus, 7);
	return cmdStatus;
}
boolean MessageComLite::getState() {
	return getStateFromCommandStatus(_commandStatus);
}
boolean MessageComLite::getStateFromCommandStatus(uint8_t cmdStatus) {
	return bitRead(cmdStatus, 7);
}

// Message Info
void MessageComLite::getMessageNumberFromMessage() {
	// read the _messageNumber from the message
	_messageNumber = _msg[3];
}
void MessageComLite::getTotalQuantityFromMessage() {
	// read the _totalQuantity from the message
	_totalQuantity = _msg[4];
}
void MessageComLite::setMessageNumber(uint8_t messageNumber) {
	_messageNumber = messageNumber;
}
void MessageComLite::setTotalQuantity(uint8_t totalQuantity) {
	_totalQuantity = totalQuantity;
}

// DataSize
void MessageComLite::getDataSizeFromMessage() {
	// read the _dataSize from the message
	_dataSize = _msg[5];
}
void MessageComLite::setDataSize(uint8_t dataSize) {
	_dataSize = dataSize;
}

// Data
void MessageComLite::getDataFromMessage() {
	// read _data from the message
	if(0 < _dataSize && (_dataSize <= _maxSize)) {
		// set pointer to the data begin of the _msg array
		_data = &_msg[6];
	}
}

boolean MessageComLite::addToData(char *value) {
	uint8_t size = strlen(value);
	uint8_t enh = extendDataTo(size);
	uint8_t pos = (_dataSize-size);
	if(enh == 2)
		_data[(pos-1)] = _delimiter;

	if(enh == 1 || enh == 2) {
		for(uint8_t i=0; i<size; i++) {
			_data[(pos+i)] = value[i];
		}
		return 1;
	}
	return 0;
}
boolean MessageComLite::addToData(char value) {
	uint8_t enh = extendDataTo(1);

	if(enh == 2)
		_data[(_dataSize-2)] = _delimiter;

	if(enh == 1 || enh == 2) {
		_data[(_dataSize-1)] = value;
		return 1;
	}
	return 0;
}
boolean MessageComLite::addToData(uint8_t value) {
	uint8_t enh = extendDataTo(1);

	if(enh == 2)
		_data[(_dataSize-2)] = _delimiter;

	if(enh == 1 || enh == 2) {
		_data[(_dataSize-1)] = value;
		return 1;
	}
	return 0;
}
boolean MessageComLite::addToData(uint16_t value) {
	uint8_t enh = extendDataTo(2);

	if(enh == 2)
		_data[(_dataSize-3)] = _delimiter;

	if(enh == 1 || enh == 2) {
		uint8_t hi = 0, lo = 0;
		lo = (uint8_t) value;
		value = value >> 8;
		hi = (uint8_t) value;

		_data[(_dataSize-2)] = hi;
		_data[(_dataSize-1)] = lo;
		return 1;
	}
	return 0;
}
boolean MessageComLite::addToData(int value) {
	uint8_t enh = extendDataTo(2);

	if(enh == 2)
		_data[(_dataSize-3)] = _delimiter;

	if(enh == 1 || enh == 2) {
		uint8_t hi = 0, lo = 0;
		lo = (uint8_t) value;
		value = value >> 8;
		hi = (uint8_t) value;
		
		_data[(_dataSize-2)] = hi;
		_data[(_dataSize-1)] = lo;
		return 1;
	}
	return 0;
}
boolean MessageComLite::addToData(long value) {
	uint8_t enh = extendDataTo(4);

	if(enh == 2)
		_data[(_dataSize-5)] = _delimiter;

	if(enh == 1 || enh == 2) {
		uint8_t hih = 0, loh = 0, hil = 0, lol = 0;
		lol = (uint8_t) value;
		value = value >> 8;
		hil = (uint8_t) value;
		value = value >> 8;
		loh = (uint8_t) value;
		value = value >> 8;
		hih = (uint8_t) value;
		
		_data[(_dataSize-4)] = hih;
		_data[(_dataSize-3)] = loh;
		_data[(_dataSize-2)] = hil;
		_data[(_dataSize-1)] = lol;
		return 1;
	}
	return 0;
}
boolean MessageComLite::addToData(unsigned long value) {
	uint8_t enh = extendDataTo(4);

	if(enh == 2)
		_data[(_dataSize-5)] = _delimiter;
	
	if(enh == 1 || enh == 2) {
		uint8_t hih = 0, loh = 0, hil = 0, lol = 0;
		lol = (uint8_t) value;
		value = value >> 8;
		hil = (uint8_t) value;
		value = value >> 8;
		loh = (uint8_t) value;
		value = value >> 8;
		hih = (uint8_t) value;
		
		_data[(_dataSize-4)] = hih;
		_data[(_dataSize-3)] = loh;
		_data[(_dataSize-2)] = hil;
		_data[(_dataSize-1)] = lol;
		return 1;
	}
	return 0;
}

uint8_t MessageComLite::getUint8FromData(uint8_t index) {
	uint8_t len;
	int start, stop;
	getPositionsOfIndexFromData(index, len, start, stop);

	return (uint8_t) _data[start];
}
char* MessageComLite::getCharArrayFromData(uint8_t index) {
	uint8_t len;
	int start, stop;
	getPositionsOfIndexFromData(index, len, start, stop);

	char result[len];

	uint8_t cnt = 0;
	for(uint8_t i=start; i<stop; i++)
		result[cnt++] = (char) _data[i];

	result[len-1] = '\0';
	return result;
}
char MessageComLite::getCharFromData(uint8_t index) {
	uint8_t len;
	int start, stop;
	getPositionsOfIndexFromData(index, len, start, stop);

	return (char) _data[start];
}
uint16_t MessageComLite::getUint16FromData(uint8_t index) {
	uint8_t len;
	int start, stop;
	getPositionsOfIndexFromData(index, len, start, stop);

	uint16_t result = (uint16_t) _data[start];
	result = result << 8;
	for(uint8_t i=0; i<8; i++)
		bitWrite(result, i, bitRead(_data[(start+1)], i));

	return result;

}
int MessageComLite::getIntFromData(uint8_t index) {
	uint8_t len;
	int start, stop;
	getPositionsOfIndexFromData(index, len, start, stop);

	int result = (int) _data[start];
	result = result << 8;
	for(uint8_t i=0; i<8; i++)
		bitWrite(result, i, bitRead(_data[(start+1)], i));

	return result;
}
long MessageComLite::getLongFromData(uint8_t index) {
	uint8_t len;
	int start, stop;
	getPositionsOfIndexFromData(index, len, start, stop);

	long result = (long) _data[start];
	result = result << 8;
	for(uint8_t i=0; i<8; i++)
		bitWrite(result, i, bitRead(_data[(start+1)], i));
	result = result << 8;
	for(uint8_t i=0; i<8; i++)
		bitWrite(result, i, bitRead(_data[(start+2)], i));
	result = result << 8;
	for(uint8_t i=0; i<8; i++)
		bitWrite(result, i, bitRead(_data[(start+3)], i));

	return result;
}
unsigned long MessageComLite::getUnsignedLongFromData(uint8_t index) {
	uint8_t len;
	int start, stop;
	getPositionsOfIndexFromData(index, len, start, stop);

	unsigned long result = (unsigned long) _data[start];
	result = result << 8;
	for(uint8_t i=0; i<8; i++)
		bitWrite(result, i, bitRead(_data[(start+1)], i));
	result = result << 8;
	for(uint8_t i=0; i<8; i++)
		bitWrite(result, i, bitRead(_data[(start+2)], i));
	result = result << 8;
	for(uint8_t i=0; i<8; i++)
		bitWrite(result, i, bitRead(_data[(start+3)], i));

	return result;
}

uint8_t MessageComLite::getDataCount() {
	int start = 0, stop = 0;
	uint8_t cnt = 0;

	while(1) {
		if((stop = indexOf(_data, _delimiter, start, _dataSize)) < 0)
			break; 
		cnt++;
		start = stop+1;
	}
	// the expression above only counts the occurrence of the delimiter
	// we have to increment it again if more than one delimiter were found
	if(cnt > 0)
		cnt++;
	return cnt;
}
uint8_t MessageComLite::firstData() {
	if(_dataCount == 0)
		_dataCount = getDataCount();
	_nextData = 0;
	return _nextData;
}
uint8_t MessageComLite::lastData() {
	if(_dataCount == 0)
		_dataCount = getDataCount();
	_nextData = (_dataCount-1);
	return _nextData;
}
uint8_t MessageComLite::prevData() {
	if(_dataCount == 0) {
		_dataCount = getDataCount();
		_nextData = 0;
	} else {
		if(0 <= (_nextData-1))
			_nextData--;
	}
	return _nextData;
}
uint8_t MessageComLite::nextData() {
	if(_dataCount == 0) {
		_dataCount = getDataCount();
		_nextData = 0;
	} else {
		if((_nextData+1) < _dataCount)
			_nextData++;
	}
	return _nextData;
}


// Checksum
void MessageComLite::getCsHFromMessage() {
	// read the _csH from the message
	_csH = _msg[(6+_dataSize)];
}
void MessageComLite::getCsLFromMessage() {
	// read the _csL from the message
	_csL = _msg[(7+_dataSize)];
}
void MessageComLite::getChecksumFromMessage() {
	// read the _csL from the message
	getCsHFromMessage();
	getCsLFromMessage();

	_checksum = (uint16_t) _csH;
	_checksum = _checksum << 8;
	for(uint8_t i=0; i<8; i++)
		bitWrite(_checksum, i, bitRead(_csL, i));
}
uint16_t MessageComLite::getChecksumFrom(uint8_t *array, uint8_t startPos) {
	uint8_t tmpPos = (startPos+array[(startPos+5)]+6);
	uint8_t csH = array[tmpPos];
	uint8_t csL = array[(tmpPos+1)];

	uint16_t checksum = (uint16_t) csH;
	checksum = checksum << 8;
	for(uint8_t i=0; i<8; i++)
		bitWrite(checksum, i, bitRead(csL, i));
	return checksum;
}
uint16_t MessageComLite::makeCrcFrom(uint8_t *array, uint8_t startPos) {
	uint16_t retval = 0xffff;
	// uint16_t retval = 0x0; // init for xmodem
	// get the checksum for the whole message
	// excluding: start-, stop-byte and the 2 checksum bytes
	// (6+_dataSize-1)
	for(uint8_t i=(startPos); i<=(startPos+5+_dataSize); i++) {
		retval = _crc_ccitt_update(retval, (uint8_t) array[i]);
		// retval = _crc_xmodem_update(retval, array[i]);
	}
	return retval;
}
void MessageComLite::setCrc() {
	_checksum = makeCrcFrom(_msg);
	getCrcLH(_checksum);
}
void MessageComLite::getCrcLH(uint16_t checksum) {
	_csL = (uint8_t) checksum;
	checksum = checksum >> 8;
	_csH = (uint8_t) checksum;
}
boolean MessageComLite::crcOk() {
	// get the transmitted checksum -> _checksum
	getChecksumFromMessage();
	// make your own checksum and compare it to the transmitted
	if(makeCrcFrom(_msg) == _checksum)
		return 1;
	return 0;
}
boolean MessageComLite::crcOk(uint8_t *array, uint8_t startPos) {
	uint16_t checksum = getChecksumFrom(array, startPos);
	// make your own checksum and compare it to the transmitted
	if(makeCrcFrom(array, startPos) == checksum)
		return 1;
	return 0;
}


void MessageComLite::gatherInfoFromMessage() {
	getVersionFromMessage();
	getTypeFromMessage();

	getCommandStatusFromMessage();
	
	getMessageNumberFromMessage();
	getTotalQuantityFromMessage();
	
	getDataSizeFromMessage();
	
	getDataFromMessage();

	getChecksumFromMessage();
}


void MessageComLite::createMessage(uint8_t taskValue, boolean state, uint8_t messageNumber, uint8_t totalQuantity) {
	_commandStatus = createCommandStatus(state, taskValue);
	_messageNumber = messageNumber;
	_totalQuantity = totalQuantity;
	createMessage();
}

void MessageComLite::createMessage() {
	// create a message and debug it.
	if((_size+8) <= _maxSize) {
		// extend the size of the message
		_size = (8+_dataSize);

		_msg[0] = _version;
		_msg[1] = _type;
		_msg[2] = _commandStatus;
		_msg[3] = _messageNumber;
		_msg[4] = _totalQuantity;
		_msg[5] = _dataSize;
		// then comes the data, usually we would copy _data to _msg, 
		// but _data shares the memory with _msg... so its not necessary

		// _checksum
		setCrc();
		_msg[(6+_dataSize)] = _csH;
		_msg[(6+_dataSize+1)] = _csL;

		_bufferSize = base64_encode(_buffer, _msg, _size, 1);
		
		_buffer[0] = _startDelimiter;
		_buffer[_bufferSize++] = _stopDelimiter;
		_buffer[_bufferSize++] = '\0';
	}
}


boolean MessageComLite::authMsg(uint8_t *array) {
	int startPos = indexOf(array, _startDelimiter, 0, _bufferMaxSize);
	if(startPos > -1) {
		// try to prevent timing errors:
		// use the 2nd message in the buffer
		int startPos2 = indexOf(array, _startDelimiter, (startPos+1), _bufferMaxSize);
		if(startPos2 > -1)
			startPos = startPos2;

		int endPos = indexOf(array, _stopDelimiter, (startPos+10), _bufferMaxSize);
		if(endPos > -1) {
			// (endPos-startPos) >= 11 // implicit true
			// start and found
			// base64-decode the message to get its content
			base64_decode(_msg, array, (endPos-startPos-1), (startPos+1));
			// get data size from message
			getDataSizeFromMessage();
			// authentificate message
			// match version
			if(_msg[0] == _version) {
				// verify the transmitted checksum
				if(crcOk(_msg)) {
					// message authentic!
					_size = (_dataSize+8);
					return 1;
				}
			}
		}
	}
	clear();
	return 0;
}

boolean MessageComLite::readMsg(uint8_t *buffer) {
	if(authMsg(buffer)) {
		gatherInfoFromMessage();
		return 1;
	}
	return 0;
}

boolean MessageComLite::recv(uint8_t maxtry, unsigned long timer, uint8_t sBytes) {
	if(sBytes > 0)
		skipBytes(sBytes);

	uint8_t recvBytePos = 0;

	for(uint8_t atry=0; atry<maxtry; atry++) {

		for(uint8_t i=0; i<_bufferMaxSize; i++)
			_buffer[i] = 0;

		// buffer the message from HW-Serial or SW-Serial
		if(_Serial != NULL) {
			if(_Serial->available()) {
				for(int i=0; i<1000; i++) {
					// read value from device
					uint8_t value = (uint8_t) _Serial->read();
					// look for startDelimiter
					if(value == _startDelimiter) {
						// start found 
						_buffer[recvBytePos++] = value;

						// just to be sure ... try many times
						for(int j=0; j<1000; j++) {
							// read value from device
							value = (uint8_t) _Serial->read();
							// look for startDelimiter ... and also for the end
							if(value == _stopDelimiter) {
								// stop found
								_buffer[recvBytePos++] = value;
								if(readMsg(_buffer))
									return 1;
							} else if(bytePlausible(value)) {
								_buffer[recvBytePos++] = value;
							}
							delay(1);
						}
						// start found once ... 
						// there is no purpose for another time to receive either the end was found or not
						recvBytePos = 0;
						break;
					}
					delay(5);
				}
			}
		} else if(_swSerial != NULL) {
			if(_swSerial->available()) {
				for(int i=0; i<1000; i++) {
					// read value from device
					uint8_t value = (uint8_t) _swSerial->read();
					// look for startDelimiter
					if(value == _startDelimiter) {
						// start found 
						_buffer[recvBytePos++] = value;

						// just to be sure ... try many times
						for(int j=0; j<1000; j++) {
							// read value from device
							value = (uint8_t) _swSerial->read();
							// look for startDelimiter ... and also for the end
							if(value == _stopDelimiter) {
								// stop found
								_buffer[recvBytePos++] = value;
								if(readMsg(_buffer))
									return 1;
							} else if(bytePlausible(value)) {
								_buffer[recvBytePos++] = value;
							}
							delay(1);
						}
						// start found once ... 
						// there is no purpose for another time to receive either the end was found or not
						recvBytePos = 0;
						break;
					}
					delay(5);
				}
			}
		}
		delay(timer*3);
	}
	return 0;
}

boolean MessageComLite::receiveAck(uint8_t sBytes) {
	if(sBytes > 0)
		skipBytes(sBytes);

	uint8_t ack = 0, nack = 0;

	for(uint8_t atry=0; atry<MCMAXTRY; atry++) {

		// buffer the message from HW-Serial or SW-Serial
		if(_Serial != NULL) {
			if(_Serial->available()) {
				for(int i=0; i<1000; i++) {
					// read value from device
					uint8_t value = (uint8_t) _Serial->read();
					// look for ack or nack
					if(value == _ackChar)
						ack++;
					else if(value == _ackChar)
						nack++;

					if(ack >= MCACKMINAMOUNT)
						return 1;
					else if(nack >= MCACKMINAMOUNT)
						return 0;

					delay(2);
				}
			}
		} else if(_swSerial != NULL) {
			if(_swSerial->available()) {
				for(int i=0; i<1000; i++) {
					// read value from device
					uint8_t value = (uint8_t) _swSerial->read();
					// look for ack or nack
					if(value == _ackChar)
						ack++;
					else if(value == _ackChar)
						nack++;

					if(ack >= MCACKMINAMOUNT)
						return 1;
					else if(nack >= MCACKMINAMOUNT)
						return 0;

					delay(2);
				}
			}
		}
		delay(MCTIMER);
	}
	return 0;
}

boolean MessageComLite::receive(uint8_t maxtry, unsigned long timer) {
	if(recv(maxtry, timer)) {
		if(getState()) {
			sendAck(1);
			return 1;
		} else {
			sendAck(0);
		}
	}
	return 0;
}

uint8_t MessageComLite::snd() {
	uint8_t sentBytes = 0;
	if(_Serial != NULL) {
		for(uint8_t i=0; i<_bufferSize; i++) {
			if(bytePlausible(_buffer[i])) {
				_Serial->write(_buffer[i]);
				sentBytes++;
			}
		}
		_Serial->println();
	} else if(_swSerial != NULL) {
		for(uint8_t i=0; i<_bufferSize; i++) {
			if(bytePlausible(_buffer[i])) {
				_swSerial->write(_buffer[i]);
				sentBytes++;
			}
		}
		_swSerial->println();
	}
	return sentBytes;
}
void MessageComLite::sendAck(boolean state) {
	if(state) {
		if(_Serial != NULL) {
			for(uint8_t i=0; i<MCACKCOUNT; i++)
				_Serial->write(_ackChar);
			_Serial->println();
		} else if(_swSerial != NULL) {
			for(uint8_t i=0; i<MCACKCOUNT; i++)
				_swSerial->write(_ackChar);
			_swSerial->println();
		}
	} else {
		if(_Serial != NULL) {
			for(uint8_t i=0; i<MCACKCOUNT; i++)
				_Serial->write(_nackChar);
			_Serial->println();
		} else if(_swSerial != NULL) {
			for(uint8_t i=0; i<MCACKCOUNT; i++)
				_swSerial->write(_nackChar);
			_swSerial->println();
		}
	}
}

boolean MessageComLite::send() {
	uint8_t skipBytes = snd();
	delay(MCTIMER);
	if(receiveAck(skipBytes)) {
		return 1;
	}
	return 0;
}
