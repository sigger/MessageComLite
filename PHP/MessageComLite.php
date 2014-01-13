<?
/*
	MessageComLite is a library to facilitate communication between devices.
	MessageComLite is useful for many platforms, including PHP and Arduino.

	The Idea of this library is based on the MessageCom library
	Link:
		https://github.com/sigger/MessageCom
	
	In this implementation of the MessageComLite library the following sources are used.

	CCITT, XMODEM X24
	Links:
		http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
		https://forums.digitalpoint.com/threads/php-define-function-calculate-crc-16-ccitt.2584389/
	
	@version 0.5
	@date 16 Nov 2013

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

// http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
// https://forums.digitalpoint.com/threads/php-define-function-calculate-crc-16-ccitt.2584389/
// CCITT, XMODEM X24
define("CRC16POLYN", 0x1021);
define("CRC16POLYI", 0x8408);

// for "STANDARD" use 0x8005 and 0xA001

function CRC16Normal($result, $buffer) {
	// xmodem
	// $result = 0x0;
	if(($length = strlen($buffer)) > 0) {
		for($offset = 0; $offset < $length; $offset++) {
			$result ^= (ord($buffer[$offset]) << 8);
			for($bitwise = 0; $bitwise < 8; $bitwise++) {
				if(($result <<= 1) & 0x10000)
					$result ^= CRC16POLYN;
				$result &= 0xFFFF; /* gut the overflow as php has no 16 bit types */
			}
		}
	}
	return $result;
}
 
function CRC16Inverse($result, $buffer) {
	// ccitt
	// $result = 0xffff;
	if(($length = strlen($buffer)) > 0) {
		for($offset = 0; $offset < $length; $offset++) {
			$result ^= ord($buffer[$offset]);
			for($bitwise = 0; $bitwise < 8; $bitwise++) {
				$lowBit = $result & 0x0001;
				$result >>= 1;
				if($lowBit)
					$result ^= CRC16POLYI;
			}
		}
	}
	return $result;
}

// bit/byte manipulation functions
function bitClear(&$value, $bitNumber) {
	$result = decbin($value);
	if($bitNumber >= strlen($result))
		for($i=0; $i<$bitNumber; $i++)
			$result .= '0';
	else
		$result = strrev($result);
	$result[$bitNumber] = 0;

	$value = bindec(strrev($result));
}
function bitSet(&$value, $bitNumber) {
	$result = decbin($value);
	if($bitNumber >= strlen($result))
		for($i=0; $i<$bitNumber; $i++)
			$result .= '0';
	else
		$result = strrev($result);
	$result[$bitNumber] = 1;

	$value = bindec(strrev($result));
}
function bitWrite(&$value, $bitNumber, $bit) {
	$result = decbin($value);
	if($bitNumber >= strlen($result))
		for($i=0; $i<$bitNumber; $i++)
			$result .= '0';
	else
		$result = strrev($result);
	$result[$bitNumber] = (boolean) $bit;

	$value = bindec(strrev($result));
}

function bitRead($value, $bitNumber) {
	$result = decbin($value);
	if($bitNumber >= strlen($result))
		return 0;
	else
		$result = strrev($result);
	return (boolean) $result[$bitNumber];
}

function getNBytesFrom($value, $n) {
// print "<pre>";
// print "str ... dechex(): ".dechex($value)."\n";
	$str = strrev(dechex($value));
// print "str ... strrev(dechex()): ".$str."\n";
	$diff = ($n*2) - strlen($str);
// print "diff: ".$diff."\n";
	if($diff != 0) {
		for($i=0; $i<$diff; $i++)
			$str .= '0';
	}
// print "str: ".$str."\n";
	$str = strrev($str);
// print "str: ".$str."\n";
	for($i=2; $i<=($n*2); $i+=2)
		$array[] = hexdec($str[$i-2].$str[$i-1]);
	
// foreach ($array as $value)
//  var_dump(dechex($value));
// print "</pre>";
	return $array;
}
function joinValue($array) {
// print "<pre>JoinValue:".PHP_EOL;
	$result = "";
	foreach($array as $value)
		if(is_string($value)) {
			if(strlen($value) == 1)
				$result .= ('0'.$value);
			else if(strlen($value) == 2)
				$result .= $value;
		} else {
			$val = dechex($value);
			if(strlen($val) == 1)
				$result .= ('0'.$val);
			else if(strlen($val) == 2)
				$result .= $val;
		}
// var_dump($result);

// print "</pre>";
// if($isstr)
//  return $result;
// else
	return hexdec($result);
}

define("MCBUFFERMAXSIZE", 256);
define("MCDEFAULTMAXSIZE", 186);
define("MCTIMER", 50);
define("MCMAXTRY", 5);
define("MCACKCOUNT", 10);
define("MCACKMINAMOUNT", 6);

class MessageComLite {
	///////////
	// private
	///////////
	private $_buffer;
	private $_msg;

	private $_data;

	// file descriptor & path
	private $_fd;
	private $_filePath;
	
	private $_dataCount;
	private $_nextData;

	private $_bufferMaxSize;
	private $_maxSize;
	private $_ackMaxSize;

	private $_size;

	private $_bufferSize;

	// Start & Stop sign
	private $_startDelimiter;
	private $_stopDelimiter;
	// data delimiter
	private $_delimiter;
	// ack sign
	private $_ackChar;
	// nack sign
	private $_nackChar;

	// identification
	private $_version;
	private $_type;

	// commandStatus
	private $_commandStatus;

	// messageInfo
	private $_messageNumber;
	private $_totalQuantity;

	// dataSize
	private $_dataSize;

	// checksum
	private $_checksum;
	private $_csH;
	private $_csL;

	private function indexOf($array, $value, $startPos=0, $endPos=0) {
		if($endPos == 0)
			$endPos = $this->_maxSize;
		for($pos=$startPos; $pos<=$endPos; $pos++) {
			if(@$array[$pos]===$value)
				return $pos;
		}
		return -1;
	}
	private function extendDataTo($bytes) {
		$retValue = 0;

		if($this->_dataSize == 0) {
			$this->_dataSize = $bytes;
			$retValue = 1;
		} else {
			$this->_dataSize += ($bytes+1);
			$retValue = 2;
		}
		if($this->_dataSize <= $this->_maxSize)
			return $retValue;
		return 0;
	}

	private function arrayToByteString($array, $len) {
		$result = "";
		for($i=0; $i<$len; $i++) {
			if(is_string($array[$i])) {
// print 'i: '.$i.' - '.$array[$i].' - '.dechex(ord($array[$i])).' - '.decbin(ord($array[$i])).' - STRING<br/ >';
				$result .=  pack('C', ord($array[$i]));
			} else {
// print 'i: '.$i.' - '.$array[$i].' - '.dechex($array[$i]).' - '.decbin($array[$i]).' - NUMBER<br/ >';
				$result .=  pack('C', $array[$i]);
			}
		}
		return $result;
	}

	private function stringToByteArray($string) {
		$result;
		for($i=0; $i<strlen($string); $i++) {
// print 'i: '.$i.' - '.$string[$i].' - '.dechex(ord($string[$i])).' - '.decbin(ord($string[$i])).' - STRING - '.dechex(($string[$i])).' - '.decbin(($string[$i])).' - NUMBER<br/ >';
			$tmp = unpack('C', $string[$i]);
			$result[] = $tmp[1];
		}
		return $result;
	}

	///////////
	// public
	///////////
	public function __construct($filePath, &$fd=NULL, $bufferMaxSize=MCBUFFERMAXSIZE, $maxSize=MCDEFAULTMAXSIZE) {
		$this->_bufferMaxSize = $bufferMaxSize;
		$this->_maxSize = $maxSize;

		$this->_startDelimiter = '#';
		$this->_stopDelimiter = ';';
		$this->_delimiter = '|';
		$this->_ackChar = '@';
		$this->_nackChar = '!';

		$this->_filePath = $filePath;
		$this->_fd = &$fd;

		$this->clear();
	}

	public function getSize() {
		return $this->_size;
	}

	// clean up
	public function clear() {
		// clear msg
		$this->_buffer = "";
		for($i=0; $i<$this->_bufferMaxSize; $i++)
			$this->_buffer .= '0';

		$this->_msg = array_fill(0, $this->_maxSize, 0);

		$this->_dataSize = 0;
		$this->_bufferSize = 0;
		$this->_size = 0;

		$this->_version = 2;
		$this->_type = 0;

		$this->_messageNumber = 1;
		$this->_totalQuantity = 1;

		$this->_dataCount = 0;
		$this->_nextData = 0;

		unset($this->_data);

		$this->_commandStatus = 0;
		$this->_checksum = 0;
		$this->_csH = 0;
		$this->_csL = 0;
	}

	// debug
	public function dPrintMsg() {
		print "<pre>";
		print "Message size: ".$this->_size." Message:".PHP_EOL;
		for($i=0; $i<$this->_size; $i++) {
			if(is_string($this->_msg[$i]))
				print 'i: '.$i.' - '.$this->_msg[$i].' - '.dechex(ord($this->_msg[$i])).' - '.decbin(ord($this->_msg[$i])).PHP_EOL;
			else
				print 'i: '.$i.' - '.$this->_msg[$i].' - '.dechex($this->_msg[$i]).' - '.decbin($this->_msg[$i]).PHP_EOL;
		}
		print "buffer:".PHP_EOL;
		var_dump($this->_buffer);

		for($i=0; $i<strlen($this->_buffer); $i++) {
			if(is_string($this->_buffer[$i]))
				print 'i: '.$i.' - '.$this->_buffer[$i].' - '.dechex(ord($this->_buffer[$i])).' - '.decbin(ord($this->_buffer[$i])).PHP_EOL;
			else
				print 'i: '.$i.' - '.$this->_buffer[$i].' - '.dechex($this->_buffer[$i]).' - '.decbin($this->_buffer[$i]).PHP_EOL;
		}
		print "obj...".PHP_EOL;
		print_r($this);
		print "</pre>";
	}
	
	public function copyArrayIntoArray($src, &$dest, $firstPos, $endPos) {
		$cnt = 0;
		for($i=$firstPos; $i<$endPos; $i++)
			$dest[$cnt++] = $src[$i];
		return 1;
	}
	public function getIndexedArrayFromData($index, &$len) {
		$start = 0; $stop = 0;
		
		for($i=0; $i<=$index; $i++) {
			if(($stop = $this->indexOf($this->_data, ord($this->_delimiter), $start, $this->_dataSize)) < 0) {
				$stop = $this->_dataSize;
				break;
			}
			if($i==$index)
				break;
			$start = $stop+1;
		}
		$len = (($stop-$start)+1);
		
		if($len > 1) {
			$this->copyArrayIntoArray($this->_data, $array, $start, $stop);
			
// print "<pre>";
// print "index: "; var_dump($index); print "<br />";
// print "start: "; var_dump($start); print "<br />";
// print "stop: "; var_dump($stop); print "<br />";
// print "len: "; var_dump($len); print "<br />";
// print "_dataSize: "; var_dump($this->_dataSize); print "<br />";
// print "array: "; var_dump($array); print "<br />";
// foreach($array as $i => $e)
//  print "i: ".$i." value: ".$e." hex_value: ".dechex($e)."<br />";
// print "</pre>";
			
			return $array;
		}
	}

	// identification
	public function getVersionFromMessage() {
		// read the $this->_version from the message
		$this->_version = $this->_msg[0];
	}
	public function getTypeFromMessage() {
		// read the $this->_type from the message
		$this->_type = $this->_msg[1];
	}
	public function setVersion($version) {
		$this->_version = $version;
	}
	public function setType($type) {
		$this->_type = $type;
	}

	// Command Status
	public function getCommandStatusFromMessage() {
		// read the $this->_commandStatus from the message
		$this->_commandStatus = $this->_msg[2];
	}
	public function setCommandStatus($state, $taskValue) {
		$this->_commandStatus = $this->createCommandStatus($state, $taskValue);
	}
	public function createCommandStatus($state, $taskValue) {
		// check the size of the task value
		// value is greater than 7-bit -> set value to maximum: 127
		if(0 > $taskValue && $taskValue >= 128)
			$taskValue = 127;
		// else: value consists of 7-bit -> everything is right

		// add state-flag (succeed-flag) to taskValue if set
		if($state)  
			$taskValue += 128;

		return $taskValue;
	}
	public function getTaskValue() {
		return $this->getTaskValueFromCommandStatus($this->_commandStatus);
	}
	public function getTaskValueFromCommandStatus($cmdStatus) {
		bitClear($cmdStatus, 7);
		return $cmdStatus;
	}
	public function getState() {
		return $this->getStateFromCommandStatus($this->_commandStatus);
	}
	public function getStateFromCommandStatus($cmdStatus) {
		return bitRead($cmdStatus, 7);
	}

	// Message Info
	public function getMessageNumberFromMessage() {
		// read the $this->_messageNumber from the message
		$this->_messageNumber = $this->_msg[3];
	}
	public function getTotalQuantityFromMessage() {
		// read the $this->_totalQuantity from the message
		$this->_totalQuantity = $this->_msg[4];
	}
	public function setMessageNumber($messageNumber) {
		$this->_messageNumber = $messageNumber;
	}
	public function setTotalQuantity($totalQuantity) {
		$this->_totalQuantity = $totalQuantity;
	}

	// DataSize
	public function getDataSizeFromMessage() {
		// read the $this->_dataSize from the message
		$this->_dataSize = $this->_msg[5];
	}
	public function setDataSize($dataSize) {
		$this->_dataSize = $dataSize;
	}

	// internal data source
	public function getDataFromMessage() {
		// read $this->_data from the message
		if(0 < $this->_dataSize && ($this->_dataSize <= $this->_maxSize)) {
			for($i=0; $i<$this->_dataSize; $i++)
				$this->_data[$i] = $this->_msg[(6+$i)];
		}
	}



	public function addToData($value) {
		if(is_int($value)) {
			if(0 <= $value && $value <= 255) {
				// uint8_t 8 bit - 1 byte
				$enh = $this->extendDataTo(1);
				if($enh == 2)
					$this->_data[($this->_dataSize-2)] = $this->_delimiter;
				if($enh == 1 || $enh == 2) {
					$this->_data[($this->_dataSize-1)] = $value;
					return 1;
				}
			} else if((0 <= $value && $value <= 65535) ||
				(-32768 <= $value && $value <= 32767)) {
				// uint16_t 16 bit - 2 byte ||
				// int 16 bit - 2 byte
				$enh = $this->extendDataTo(2);
				if($enh == 2)
					$this->_data[($this->_dataSize-3)] = $this->_delimiter;
				if($enh == 1 || $enh == 2) {

					$arr = getNBytesFrom($value, 2);

					$this->_data[($this->_dataSize-2)] = $arr[0];
					$this->_data[($this->_dataSize-1)] = $arr[1];
					return 1;
				}
			} else if(-2147483648 <= $value && $value <= 2147483647) {
				// long 32 bit - 4 byte
				$enh = $this->extendDataTo(4);
				if($enh == 2)
					$this->_data[($this->_dataSize-5)] = $this->_delimiter;
				if($enh == 1 || $enh == 2) {
					$arr = getNBytesFrom($value, 4);

					$this->_data[($this->_dataSize-4)] = $arr[0];
					$this->_data[($this->_dataSize-3)] = $arr[1];
					$this->_data[($this->_dataSize-2)] = $arr[2];
					$this->_data[($this->_dataSize-1)] = $arr[3];

					return 1;
				}
			}
		} else if (is_float($value)) {
			if(4.2 < $value) {
				// vermutlich
				// unsigned long 32 bit - 4 byte
				$enh = $this->extendDataTo(4);
				if($enh == 2)
					$this->_data[($this->_dataSize-3)] = $this->_delimiter;
				if($enh == 1 || $enh == 2) {
					$arr = getNBytesFrom($value, 4);

					$this->_data[($this->_dataSize-4)] = $arr[0];
					$this->_data[($this->_dataSize-3)] = $arr[1];
					$this->_data[($this->_dataSize-2)] = $arr[2];
					$this->_data[($this->_dataSize-1)] = $arr[3];
					return 1;
				}
			}
		} else if(is_bool($value) === true) {
			// uint8_t 8 bit - 1 byte
				$enh = $this->extendDataTo(1);
				if($enh == 2)
					$this->_data[($this->_dataSize-2)] = $this->_delimiter;
				if($enh == 1 || $enh == 2) {
					$this->_data[($this->_dataSize-1)] = $value;
					return 1;
				}
		} else if(is_string($value)) {
			// pro char je uint8_t 8 bit - 1 byte
			$size = strlen($value);
			$enh = $this->extendDataTo($size);
			$pos = ($this->_dataSize-$size);
			if($enh == 2)
				$this->_data[($pos-1)] = $this->_delimiter;

			if($enh == 1 || $enh == 2) {
				for($i=0; $i<$size; $i++) {
					$this->_data[($pos+$i)] = $value[$i];
				}
				return 1;
			}
		}
		return 0;
	}
	public function getValueFromData($index) {
		$len = 0;
		$array = $this->getIndexedArrayFromData($index, $len);
		if(count($array) > 0)
			return $array;
	}
	
	public function getUint8FromData($index) {
		$len = 0;
		$array = $this->getIndexedArrayFromData($index, $len);
		if(count($array) > 0)
			return $array[0];
	}
	public function getStringFromData($index) {
		$array = $this->getCharArrayFromData($index);
		$result = "";
		if(count($array) > 0) {
			foreach ($array as $elem)
				if($elem != '\0')
					$result .= $elem;
			return $result;
		}
	}
	public function getCharArrayFromData($index) {
		$len = 0;
		$array = $this->getIndexedArrayFromData($index, $len);
		if(count($array) > 0) {
			foreach($array as $val)
				$result[] = chr($val);
			$result[$len-1] = '\0';
			return $result;
		}
	}
	public function getCharFromData($index) {
		$len = 0;
		$array = $this->getIndexedArrayFromData($index, $len);
		if(count($array) > 0)
			return $array[0];
	}
	public function getNumberFromData($index) {
		$len = 0;
		$array = $this->getIndexedArrayFromData($index, $len);

// print "<pre>";
// print "getNumberFromData:".PHP_EOL;
// print_r($array);
// print "</pre>";

		if(count($array) > 0)
			return joinValue($array);
	}
	public function getUint16FromData($index) {
		return $this->getNumberFromData($index);
	}
	public function getIntFromData($index) {
		return $this->getNumberFromData($index);
	}
	public function getLongFromData($index) {
		return $this->getNumberFromData($index);
	}
	public function getUnsignedLongFromData($index) {
		return $this->getNumberFromData($index);
	}
	

	public function getDataCount() {
		$start = 0; $stop = 0;
		$cnt = 0;
		if($this->_dataSize == 1) {
			// no need to search, there will be no _delimiter found
			return 1;
		} else if($this->_dataSize > 1) {
			while(1) {
				if(($stop = $this->indexOf($this->_data, ord($this->_delimiter), $start, $this->_dataSize)) < 0)
					break; 
				$cnt++;
				$start = $stop+1;
			}
			// the expression above only counts the occurrence of the delimiter
			// we have to increment it again if more than one delimiter were found
			return ++$cnt;
		}
		// no need to search, no data available
		return 0;
	}
	public function firstData() {
		if($this->_dataCount == 0)
			$this->_dataCount = $this->getDataCount();
		$this->_nextData = 0;
		return $this->_nextData;
	}
	public function lastData() {
		if($this->_dataCount == 0)
			$this->_dataCount = $this->getDataCount();
		$this->_nextData = ($this->_dataCount-1);
		return $this->_nextData;
	}
	public function prevData() {
		if($this->_dataCount == 0) {
			$this->_dataCount = $this->getDataCount();
			$this->_nextData = 0;
		} else {
			if(0 <= ($this->_nextData-1))
				$this->_nextData--;
		}
		return $this->_nextData;
	}
	public function nextData() {
		if($this->_dataCount == 0) {
			$this->_dataCount = $this->getDataCount();
			$this->_nextData = 0;
		} else {
			if(($this->_nextData+1) < $this->_dataCount)
				$this->_nextData++;
		}
		return $this->_nextData;
	}


	// Checksum
	public function getCsHFromMessage() {
		// read the $this->_csH from the message
		$this->_csH = $this->_msg[(6+$this->_dataSize)];
	}
	public function getCsLFromMessage() {
		// read the $this->_csL from the message
		$this->_csL = $this->_msg[(7+$this->_dataSize)];
	}
	public function getChecksumFromMessage() {
		// read the $this->_csL from the message
		$this->getCsHFromMessage();
		$this->getCsLFromMessage();


		$arr[] = $this->_csH;
		$arr[] = $this->_csL;
		
		$this->_checksum = joinValue($arr);
	}
	public function getChecksumFrom($array, $startPos=0) {
		// read the $this->_csL from the message
// print "getChecksumFrom relative POS: ";
		$tmpPos = ($startPos+($array[($startPos+5)])+6);
// var_dump($tmpPos);
		$csH = $array[$tmpPos];
		$csL = $array[($tmpPos+1)];

		$arr[] = $csH;
		$arr[] = $csL;

		return joinValue($arr);
	}
	public function makeCrcFrom($array, $startPos=0) {
		$retval = 0xffff;
		// get the checksum for the whole message
		for($i=($startPos); $i<=($startPos+5+$this->_dataSize); $i++) {
// print "<pre>";
// var_dump(dechex($retval));
// print "</pre>";
			if(!is_string($array[$i]))
				$value = chr($array[$i]);
			else
				$value = $array[$i];

			$retval = CRC16Inverse($retval, $value);
		}
// print "<pre>return value:<br />";
// var_dump(dechex($retval));
// var_dump(($retval));

// print "</pre>";
		return $retval;
	}
	public function setCrc() {
		$this->_checksum = $this->makeCrcFrom($this->_msg);
		$this->getCrcLH($this->_checksum);
	}
	public function getCrcLH($checksum) {
// print "<pre>getCrcLH:<br />";
// print "checksum<br />";
// var_dump(dechex($checksum));
// var_dump(($checksum));
// print "csl,csh<br />";

// var_dump(dechex($this->_csL));
// var_dump(($this->_csL));
// var_dump(dechex($this->_csH));
// var_dump(($this->_csH));
// print "</pre>";
		
		$str = dechex($checksum);
		if(strlen($str) < 4) {
			$str = strrev($str);
			for($i=strlen($str); $i<4; $i++)
				$str .= '0';
			$str = strrev($str);
		}
		
		$this->_csL = "";
		$this->_csL .= $str[2];
		$this->_csL .= $str[3];
		$this->_csL = hexdec($this->_csL);
		
		$this->_csH = "";
		$this->_csH .= $str[0];
		$this->_csH .= $str[1];
		$this->_csH = hexdec($this->_csH);
		
// print "<pre>";
// print "csl,csh<br />";
// var_dump(dechex($this->_csL));
// var_dump(($this->_csL));
// var_dump(dechex($this->_csH));
// var_dump(($this->_csH));
// print "</pre>";
	}
	public function crcOk($array=-1) {
		if($array == -1) {
			// get the transmitted checksum -> $this->_checksum
			$this->getChecksumFromMessage();
			// make your own checksum and compare it to the transmitted
			if($this->makeCrcFrom($this->_msg) == $this->_checksum)
				return 1;
		} else {
			$checksum = $this->getChecksumFrom($array);
			// make your own checksum and compare it to the transmitted
			if($this->makeCrcFrom($array) == $checksum)
				return 1;
		}
		return 0;
	}


	public function gatherInfoFromMessage() {
		$this->getVersionFromMessage();
		$this->getTypeFromMessage();

		$this->getCommandStatusFromMessage();
		
		$this->getMessageNumberFromMessage();
		$this->getTotalQuantityFromMessage();
		
		$this->getDataSizeFromMessage();
		
		$this->getDataFromMessage();

		$this->getChecksumFromMessage();
	}



	public function createMessage($taskValue=-1, $state=-1, $messageNumber=1, $totalQuantity=1) {
// print "createMessage: ";
// var_dump($taskValue);
// var_dump($state);
		if($taskValue != -1 && $state != -1)
			$this->_commandStatus = $this->createCommandStatus($state, $taskValue);

		$this->_messageNumber = $messageNumber;
		$this->_totalQuantity = $totalQuantity;

		// create a message and debug it.
		if(($this->_size+8) <= $this->_maxSize) {

			$this->_size = (8+$this->_dataSize);

			$this->_msg[0] = $this->_version;
			$this->_msg[1] = $this->_type;
			$this->_msg[2] = $this->_commandStatus;
			$this->_msg[3] = $this->_messageNumber;
			$this->_msg[4] = $this->_totalQuantity;
			$this->_msg[5] = $this->_dataSize;
			for($i=0; $i<$this->_dataSize; $i++) {
// print "<pre>";
// print "@createMessage, this->_data[i]:<br />";
// var_dump($this->_data[$i]);
// print "</pre>";
				$this->_msg[(6+$i)] = $this->_data[$i];
			}
// for($i=0; $i<$this->_dataSize; $i++) {
//  // print "<pre>";
//  // print "@createMessage, this->_data[i]:<br />";
//  // var_dump($this->_data[$i]);
//  // print "</pre>";
//  $this->_msg[(6+$i)] = $this->_data[$i];
// }
			$this->setCrc();
			$this->_msg[(6+$this->_dataSize)] = $this->_csH;
			$this->_msg[(6+$this->_dataSize+1)] = $this->_csL;

			$str = base64_encode($this->arrayToByteString($this->_msg, $this->_size));
			$this->_buffer = ($this->_startDelimiter.$str.$this->_stopDelimiter);
			$this->_bufferSize = strlen($this->_buffer);
		}
	}




	public function authMsg($array) {
		$firstPos = $this->indexOf($array, $this->_startDelimiter, 0, strlen($array)); // $this->_bufferMaxSize);
		if($firstPos > -1) {
// print "<pre>start found?: ".$firstPos."</pre>";
			// start char found
			$nextPos = $this->indexOf($array, $this->_stopDelimiter, ($firstPos+10), strlen($array)); // $this->_bufferMaxSize);
			if($nextPos > -1) {
// print "<pre>end found?: ".$nextPos."</pre>";
				// maybe stop char found
				// check the length of the message
				if(($nextPos-$firstPos) >= 11) {
// print "<pre>min Dist ok!: ".($nextPos-$firstPos)."</pre>";

					// authentificate message
					// match version
// var_dump($array);
					$str = base64_decode($array);
// var_dump($str);

					// convert string to 1 byte array
					$this->_msg = $this->stringToByteArray($str);
					// get data size from message
					$this->getDataSizeFromMessage();
					$this->_size = (8+$this->_dataSize);

// print "now PRINT!   ";
// $this->dPrintMsg();
					if($this->_msg[0] == $this->_version) {
						// verify the transmitted checksum
						if($this->crcOk($this->_msg)) {
// print "<pre>message authentic!</pre>";
							// message authentic!
							// copy message into $this->_msg
							$this->_size = ($this->_dataSize+8);
							return 1;
						} 
// else {
//  print "<pre>message NOT authentic ... CRC bad?</pre>";
// }
					} 
// else {
//  print "<pre>_msg[0] == _version: </pre>".($this->_msg[0] == $this->_version);
//  print "<pre>_msg[0]: </pre>".$this->_msg[0];
//  print "<pre>_version: </pre>".$this->_version;
// }
				}
			} 
		}
		$this->clear();
		return 0;
	}
	public function readMsg($buffer) {
		if($this->authMsg($buffer)) {
			$this->gatherInfoFromMessage();
			return 1;
		}
		return 0;
	}
	public function fopen() {
		if(!$this->_fd) {
			if(!@$this->_fd = fopen($this->_filePath, 'r+b'))
				die("Could not open $this->_filePath in READ/WRITE mode");
			stream_set_blocking($this->_fd, 1);
			usleep(MCTIMER*1000);
		}
	}
	public function fclose() {
		@fclose($this->_fd);
	}

	public function recv() {
// print "recv: ".PHP_EOL;
		// read a EOL or a message...
		$this->_buffer = fgets($this->_fd);
		// read a EOL or a message...
		$this->_buffer .= fgets($this->_fd);

		// check for the minimal buffer length
		if(strlen($this->_buffer) >= 11) {
			// look for the start sign
			$startPos = strpos($this->_buffer, $this->_startDelimiter);
			if($startPos !== false) {
				// look for the stop sign
				$stopPos = strpos($this->_buffer, $this->_stopDelimiter, ($startPos+1));
				if($stopPos !== false) {
					if($this->readMsg($this->_buffer)) {
// print "RECV erfolgreich .... ".PHP_EOL;
						return 1;
					} 
				}
			}
		}
// print "recv: negative".PHP_EOL;
		$this->_buffer = "";
		return 0;
	}

	public function receiveAck() {
// print "receiveAck... ".PHP_EOL;

		$tmpBuf = fgets($this->_fd);

		if(strlen($tmpBuf) > 0) {
			// look for ack or nack
			if(substr_count($tmpBuf, $this->_ackChar) >= MCACKMINAMOUNT)
				return 1;
			else if(substr_count($tmpBuf, $this->_nackChar) >= MCACKMINAMOUNT)
				return 0;
		}
	}

	public function receive() {
		if($this->recv()) {
// print "receive, recv OK!!!! ";
			if($this->getState()) {
				$this->sendAck(1);
				return 1;
			} else {
				$this->sendAck(0);
			}
		}
		return 0;
	}

	public function snd() {
// print "send: ";
// var_dump($this->_buffer);
// $this->dPrintMsg();
		fputs($this->_fd, PHP_EOL);
		fputs($this->_fd, $this->_buffer, strlen($this->_buffer));
		fputs($this->_fd, PHP_EOL);
	}
	public function sendAck($state) {
		if($state) {
			fputs($this->_fd, PHP_EOL);
			for($i=0; $i<MCACKCOUNT; $i++)
				fputs($this->_fd, $this->_ackChar);
			fputs($this->_fd, PHP_EOL);   
		} else {
			fputs($this->_fd, PHP_EOL);
			for($i=0; $i<MCACKCOUNT; $i++)
				fputs($this->_fd, $this->_nackChar);
			fputs($this->_fd, PHP_EOL);   
		}
	}

	public function send() {
		$this->snd();
		usleep((MCTIMER*1000)*5);
		if($this->receiveAck()) {
			return 1;
		}
		return 0;
	}
}

?>
