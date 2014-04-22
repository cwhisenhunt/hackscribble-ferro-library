/*

	Hackscribble_Ferro Library
	==========================
	
	Connects Fujitsu Ferroelectric RAM (MB85RS range) to your 
	Arduino to add up to 32KB of fast, non-volatile storage.
	
	For information on how to install and use the library,
	read "Hackscribble_Ferro user guide.md".
	
	
	Created on 18 April 2014
	By Ray Benitez
	Last modified on ---
	By ---
	Change history in "README.md"
		
	This software is licensed by Ray Benitez under the MIT License.
	
	git@hackscribble.com | http://www.hackscribble.com | http://www.twitter.com/hackscribble

*/

#ifndef HACKSCRIBBLE_FERRO_H
#define HACKSCRIBBLE_FERRO_H

#include "Arduino.h"
#include <SPI.h>


// MB85RS part numbers

enum ferroPartNumber
{
	MB85RS16 = 0,
	MB85RS64,
	MB85RS128A,
	MB85RS128B,
	MB85RS256A,
	MB85RS256B,
	numberOfPartNumbers
};


// Result codes

enum ferroResult
{
	ferroOK = 0,
	ferroBadStartAddress,
	ferroBadNumberOfBytes,
	ferroBadFinishAddress,
	ferroArrayElementTooBig,
	ferroBadArrayIndex,
	ferroBadArrayStartAddress,
	ferroBadResponse,
	ferroUnknownError = 99
};


class Hackscribble_Ferro
{
private:

	ferroPartNumber _partNumber;
	byte _chipSelect;
	
	// FRAM opcodes
	static const byte _WREN = 0x06;
	static const byte _WRDI = 0x04;
	static const byte _WRITE = 0x02;
	static const byte _READ = 0x03;
	static const byte _RDSR = 0x05;
	static const byte _WRSR = 0x01;

	// Dummy write value for SPI read
	static const byte _dummy = 0x00;

	// Set maximum size of buffer used to write to and read from FRAM
	// Do not exceed 0x80 to prevent problems with maximum size structs in FerroArray
	static const unsigned int _maxBufferSize = 0x40;

	// Used in constructor to set size of usable FRAM memory, reserving some bytes as a control block
	/* static const*/ unsigned int _topAddressForPartNumber[numberOfPartNumbers];
	unsigned int _baseAddress;
	unsigned int _bottomAddress;
	unsigned int _topAddress;
	unsigned int _numberOfBuffers;
	
	// FRAM current next byte to allocate
	unsigned int _nextFreeByte;

	void _select();
	void _deselect();

public:

	Hackscribble_Ferro(ferroPartNumber partNumber = MB85RS128A, byte chipSelect = SS);
	ferroResult begin();
	ferroPartNumber getPartNumber();
	unsigned int getMaxBufferSize();
	unsigned int getBottomAddress();
	unsigned int getTopAddress();
	ferroResult checkForFRAM();
	unsigned int getControlBlockSize();
	void writeControlBlock(byte *buffer);
	void readControlBlock(byte *buffer);
	ferroResult read(unsigned int startAddress, byte numberOfBytes, byte *buffer);
	ferroResult write(unsigned int startAddress, byte numberOfBytes, byte *buffer);
	unsigned int allocateMemory(unsigned int numberOfBytes, ferroResult& result);
	ferroResult format();

};


class Hackscribble_FerroArray
{
private:

	unsigned int _numberOfElements;
	byte _sizeOfElement;
	unsigned int _startAddress;
	Hackscribble_Ferro& _f;
	
public:
	
	Hackscribble_FerroArray(Hackscribble_Ferro& f, unsigned int numberOfElements, byte sizeOfElement, ferroResult &result);
	void readElement(unsigned int index, byte *buffer, ferroResult &result);
	void writeElement(unsigned int index, byte *buffer, ferroResult &result);
	unsigned int getStartAddress();
	
};


#endif
