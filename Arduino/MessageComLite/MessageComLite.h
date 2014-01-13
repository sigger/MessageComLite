/*
  MessageComLite.h
	
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

#ifndef MessageComLite_h
#define MessageComLite_h

#include <Arduino.h>
#include <stdlib.h>
#include <SoftwareSerial.h>
#include <Base64.h>
#include <util/crc16.h>


#define MCTIMER 50
#define MCMAXTRY 5
#define MCACKCOUNT 10
#define MCACKMINAMOUNT 6

class MessageComLite {
	private:
		int indexOf(uint8_t*, uint8_t, uint8_t=0, uint8_t=0);
		uint8_t extendDataTo(uint8_t);
		void getPositionsOfIndexFromData(uint8_t, uint8_t&, int&, int&);
		boolean bytePlausible(uint8_t);
		void skipBytes(uint8_t);

		// POINTER
		// pointer to extern buffer array
		uint8_t* _buffer;
		// pointer to extern msg array
		uint8_t* _msg;

		// pointer to data part of the message
		uint8_t* _data;

		// communication interfaces HW- or SW-Serial
		HardwareSerial* _Serial;
		SoftwareSerial* _swSerial;

		// used for data extraction
		uint8_t _dataCount;
		uint8_t _nextData;

		// maximum size of the whole buffer
		uint8_t _bufferMaxSize;
		// maximum size of the whole message
		uint8_t _maxSize;
		// maximum size of the whole ack-message
		uint8_t _ackMaxSize;

		// actual size of the whole message
		uint8_t _size;

		// actual size of the whole message (base64)
		uint8_t _bufferSize;

		// Start & Stop sign
		// char _authDelimiter;
		char _startDelimiter;
		char _stopDelimiter;

		// data delimiter
		char _delimiter;

		// ack sign
		char _ackChar;
		// nack sign
		char _nackChar;

		// identification
		uint8_t _version;
		uint8_t _type;

		// commandStatus
		uint8_t _commandStatus;

		// messageInfo
		uint8_t _messageNumber;
		uint8_t _totalQuantity;

		// actual size of the "_data array"
		uint8_t _dataSize;

		// checksum
		uint16_t _checksum;
		uint8_t _csH;
		uint8_t _csL;
	public:
		MessageComLite(HardwareSerial&, uint8_t*, uint8_t, uint8_t*, uint8_t);
		MessageComLite(SoftwareSerial&, uint8_t*, uint8_t, uint8_t*, uint8_t);

		uint8_t getSize();

		// clean up the message ... reset values
		void clear();

		// identification methods
		void getVersionFromMessage();
		void getTypeFromMessage();
		void setVersion(uint8_t);
		void setType(uint8_t);


		// Command Status methods
		void getCommandStatusFromMessage();
		void setCommandStatus(boolean, uint8_t);
		uint8_t createCommandStatus(boolean, uint8_t);
		uint8_t getTaskValue();
		uint8_t getTaskValueFromCommandStatus(uint8_t);
		boolean getState();
		boolean getStateFromCommandStatus(uint8_t);

		// Message Info methods
		void getMessageNumberFromMessage();
		void getTotalQuantityFromMessage();
		void setMessageNumber(uint8_t);
		void setTotalQuantity(uint8_t);

		// DataSize methods
		void getDataSizeFromMessage();
		void setDataSize(uint8_t);

		// Data methods
		void getDataFromMessage();

		boolean addToData(char*);
		boolean addToData(char);
		boolean addToData(uint8_t);
		boolean addToData(uint16_t);
		boolean addToData(int);
		boolean addToData(long);
		boolean addToData(unsigned long);

		uint8_t getUint8FromData(uint8_t);
		char* getCharArrayFromData(uint8_t);
		char getCharFromData(uint8_t);
		uint16_t getUint16FromData(uint8_t);
		int getIntFromData(uint8_t);
		long getLongFromData(uint8_t);
		unsigned long getUnsignedLongFromData(uint8_t);
		
		// data pointing methods
		uint8_t getDataCount();
		uint8_t firstData();
		uint8_t lastData();
		uint8_t prevData();
		uint8_t nextData();

		// Checksum methods
		void getCsHFromMessage();
		void getCsLFromMessage();
		void getChecksumFromMessage();
		uint16_t getChecksumFrom(uint8_t*, uint8_t=0);
		uint16_t makeCrcFrom(uint8_t*, uint8_t=0);
		void setCrc();
		void getCrcLH(uint16_t);
		boolean crcOk();
		boolean crcOk(uint8_t*, uint8_t=0);

		// modeling and extraction methods
		void gatherInfoFromMessage();

		void createMessage(uint8_t, boolean, uint8_t=1, uint8_t=1);
		void createMessage();

		// verification and communication methods
		boolean authMsg(uint8_t*);
		boolean readMsg(uint8_t*);

		boolean recv(uint8_t=MCMAXTRY, unsigned long=MCTIMER, uint8_t=0);
		boolean receiveAck(uint8_t);
		boolean receive(uint8_t=MCMAXTRY, unsigned long=MCTIMER);

		uint8_t snd();
		void sendAck(boolean);
		boolean send();
};

#endif
