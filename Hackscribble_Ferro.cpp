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

 

#include "Arduino.h"
#include <Hackscribble_Ferro.h>



void Hackscribble_Ferro::_select()
{
	digitalWrite(_chipSelect, LOW);
}

void Hackscribble_Ferro::_deselect()
{
	digitalWrite(_chipSelect, HIGH);
}

Hackscribble_Ferro::Hackscribble_Ferro(ferroPartNumber partNumber, byte chipSelect): _partNumber(partNumber), _chipSelect(chipSelect)
{
	_topAddressForPartNumber[MB85RS16]		= 0x07FF;
	_topAddressForPartNumber[MB85RS64]		= 0x1FFF;
	_topAddressForPartNumber[MB85RS128A]	= 0x3FFF;
	_topAddressForPartNumber[MB85RS128B]	= 0x3FFF;
	_topAddressForPartNumber[MB85RS256A]	= 0x7FFF;
	_topAddressForPartNumber[MB85RS256B]	= 0x7FFF;
																	
	_baseAddress = 0x0000;
	_bottomAddress = _baseAddress + _maxBufferSize;
	_topAddress = _topAddressForPartNumber[_partNumber];
	_numberOfBuffers = (_topAddress - _bottomAddress + 1) / _maxBufferSize;
	
	// _chipSelect = chipSelect;
	// Set the standard SS pin as an output to keep Arduino SPI happy
	pinMode (SS, OUTPUT);
	// Set CS to inactive
	pinMode (chipSelect, OUTPUT);
	_deselect();
	_nextFreeByte = _bottomAddress;
}

ferroResult Hackscribble_Ferro::begin()
{
	// Initialize SPI
	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode (SPI_MODE0);
	SPI.setClockDivider(SPI_CLOCK_DIV2);
		
	// Check that the FRAM is reachable
	return checkForFRAM();
	
}
	
ferroPartNumber Hackscribble_Ferro::getPartNumber()
{
	return _partNumber;
}

unsigned int Hackscribble_Ferro::getMaxBufferSize()
{
	return _maxBufferSize;
}

unsigned int Hackscribble_Ferro::getBottomAddress()
{
	return _bottomAddress;
}

unsigned int Hackscribble_Ferro::getTopAddress()
{
	return _topAddress;
}


ferroResult Hackscribble_Ferro::checkForFRAM()
{
	// Tests that the unused status register bits can be read, inverted, written back and read again
		
	const byte srMask = 0x78; // Unused bits are bits 6..4
	byte registerValue = 0;
	byte newValue = 0;
	boolean isPresent = true;
		
	// Read current value
	_select();
	SPI.transfer(_RDSR);
	registerValue = SPI.transfer(_dummy);
	_deselect();
		
	// Invert current value
	newValue = registerValue ^ srMask;
		
	// Write new value
	_select();
	SPI.transfer(_WREN);
	_deselect();
	_select();
	SPI.transfer(_WRSR);
	SPI.transfer(newValue);
	_deselect();
		
	// Read again
	_select();
	SPI.transfer(_RDSR);
	registerValue = SPI.transfer(_dummy);
	_deselect();
		
	if (((registerValue & srMask) == (newValue & srMask)))
	{
		return ferroOK;
	}
	else
	{
		return ferroBadResponse;	
	}
		
}
	
	
unsigned int Hackscribble_Ferro::getControlBlockSize()
{
	return _maxBufferSize;
}

void Hackscribble_Ferro::writeControlBlock(byte *buffer)
{
	_select();
	SPI.transfer(_WREN);
	_deselect();

	_select();
	SPI.transfer(_WRITE);
		
	SPI.transfer(_baseAddress / 256);
	SPI.transfer(_baseAddress % 256);
		
	for (byte i = 0; i < _maxBufferSize; i++)
	{
		SPI.transfer(buffer[i]);
	}
		
	_deselect();

}


void Hackscribble_Ferro::readControlBlock(byte *buffer)
{

	_select();
	SPI.transfer(_READ);

	SPI.transfer(_baseAddress / 256);
	SPI.transfer(_baseAddress % 256);

	for (byte i = 0; i < _maxBufferSize; i++)
	{
		buffer[i] = SPI.transfer(_dummy);
	}

	_deselect();
		
}
	

ferroResult Hackscribble_Ferro::read(unsigned int startAddress, byte numberOfBytes, byte *buffer)
{
	// Copies numberOfBytes bytes from FRAM (starting at startAddress) into buffer (starting at 0)
	// Returns result code
		
	// Validations:
	//		_bottomAddress <= startAddress <= _topAddress
	//		0 < numberOfBytes <= maxBuffer
	//		startAddress + numberOfBytes <= _topAddress
		
	if ((startAddress < _bottomAddress) || (startAddress > _topAddress))
	{
		return ferroBadStartAddress;
	}
	if ((numberOfBytes > _maxBufferSize) || (numberOfBytes == 0))
	{
		return ferroBadNumberOfBytes;
	}
	if ((startAddress + numberOfBytes - 1) > _topAddress)
	{
		return ferroBadFinishAddress;
	}
		
	_select();
	SPI.transfer(_READ);
	SPI.transfer(startAddress / 256);
	SPI.transfer(startAddress % 256);
	for (byte i = 0; i < numberOfBytes; i++)
	{
		buffer[i] = SPI.transfer(_dummy);
	}
	_deselect();
		
	return ferroOK;
}

ferroResult Hackscribble_Ferro::write(unsigned int startAddress, byte numberOfBytes, byte *buffer)
{
	// Copies numberOfBytes bytes from buffer (starting at 0) into FRAM (starting at startAddress)
	// Returns result code
		
	// Validations:
	//		_bottomAddress <= startAddress <= _topAddress
	//		0 < numberOfBytes <= maxBuffer
	//		startAddress + numberOfBytes - 1 <= _topAddress
		
	if ((startAddress < _bottomAddress) || (startAddress > _topAddress))
	{
		return ferroBadStartAddress;
	}
	if ((numberOfBytes > _maxBufferSize) || (numberOfBytes == 0))
	{
		return ferroBadNumberOfBytes;
	}
	if ((startAddress + numberOfBytes - 1) > _topAddress)
	{
		return ferroBadFinishAddress;
	}
		
	_select();
	SPI.transfer(_WREN);
	_deselect();

	_select();
	SPI.transfer(_WRITE);
	SPI.transfer(startAddress / 256);
	SPI.transfer(startAddress % 256);
	for (byte i = 0; i < numberOfBytes; i++)
	{
		SPI.transfer(buffer[i]);
	}
	_deselect();

	return ferroOK;
}


unsigned int Hackscribble_Ferro::allocateMemory(unsigned int numberOfBytes, ferroResult& result)
{
	if ((_nextFreeByte + numberOfBytes) < _topAddress)
	{
		unsigned int base = _nextFreeByte;
		_nextFreeByte += numberOfBytes;
		result = ferroOK;
		return base;
	}
	else
	{
		result = ferroBadFinishAddress;
		return 0;
	}
		
}


ferroResult Hackscribble_Ferro::format()
{
	// Fills FRAM with 0 but does NOT overwrite control block
	// Returns result code from ferroWrite function, or ferroOK if format is successful

	byte buffer[_maxBufferSize];
		
	for (byte i = 0; i < _maxBufferSize; i++)
	{
		buffer[i] = 0;
	}
		
	ferroResult result = ferroOK;
	unsigned int i = _bottomAddress;
	while ((i < _topAddress) && (result == ferroOK))
	{
		result = write(i, _maxBufferSize, buffer);
		i += _maxBufferSize;
	}
	return result;
}



Hackscribble_FerroArray::Hackscribble_FerroArray(Hackscribble_Ferro& f, unsigned int numberOfElements, byte sizeOfElement, ferroResult &result): _f(f), _numberOfElements(numberOfElements), _sizeOfElement(sizeOfElement)
{
	// Creates array in FRAM
	// Calculates and allocates required memory
	// Returns result code
		
	// Validations:
	//		_sizeOfElement <= _bufferSize
		
	if (_sizeOfElement < _f.getMaxBufferSize())
	{
		_startAddress = _f.allocateMemory(_numberOfElements * _sizeOfElement, result);
	}
	else
	{
		result = ferroArrayElementTooBig;
		_startAddress = 0;
	}
		
}
	
void Hackscribble_FerroArray::readElement(unsigned int index, byte *buffer, ferroResult &result)
{
	// Reads element from array in FRAM
	// Returns result code
		
	// Validations:
	//		_startAddress > 0 (otherwise array has probably not been created)
	//		index < _numberOfElements
		
	if (_startAddress == 0)
	{
		result = ferroBadArrayStartAddress;
	}
	else if (index >= _numberOfElements)
	{
		result = ferroBadArrayIndex;
	}
	else
	{
		result = _f.read(_startAddress + (index * _sizeOfElement), _sizeOfElement, buffer);
	}
}
	
void Hackscribble_FerroArray::writeElement(unsigned int index, byte *buffer, ferroResult &result)
{
	// Writes element to array in FRAM
	// Returns result code
		
	// Validations:
	//		_startAddress > 0 (otherwise array has probably not been created)
	//		index < _numberOfElements
		
	if (_startAddress == 0)
	{
		result = ferroBadArrayStartAddress;
	}
	else if (index >= _numberOfElements)
	{
		result = ferroBadArrayIndex;
	}
	else
	{
		result = _f.write(_startAddress + (index * _sizeOfElement), _sizeOfElement, buffer);
	}
}
	
unsigned int Hackscribble_FerroArray::getStartAddress()
{
	return _startAddress;
}
