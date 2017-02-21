#include <windows.h>
#include "ViewerOutput.h"

ViewerOutput& ViewerOutput::Inst()
{
	static ViewerOutput inst;
	return inst;
}

void ViewerOutput::RegisterPrinter( FrontPrinter* printer )
{
	if(!printer) return;

	_printers.insert(printer);
}

void ViewerOutput::UnRegisterPrinter( FrontPrinter* printer )
{
	if(!printer) return;

	auto itr = _printers.find(printer);
	if(itr != _printers.end())
		_printers.erase(itr);
}

void ViewerOutput::print( FrontPrinter::Effect e, unsigned long color, const char* fmt, ... )
{
	va_list args;
	va_start(args, fmt);

	vsprintf_s(_buf, MAX_BUFFER_SIZE, fmt, args);

	va_end(args);

	for (auto itr = _printers.begin(); itr != _printers.end(); ++itr)
	{
		(*itr)->print(_buf, color, e);
	}
}

//////////////////////////////////////////////////////////////////////////
ViewerStream::~ViewerStream()
{
	flush();
}

ViewerStream& ViewerStream::flush()
{
	this->std::ostream::flush();

	if(_buffer.size() == 0)
		return (*this);

	ViewerOutput::Inst().print(_effect, _color, _buffer.buf);
	_buffer.reset();

	return (*this);
}

ViewerStream::ViewerStreamBuf::ViewerStreamBuf()
{
	reset();
}

std::streamsize ViewerStream::ViewerStreamBuf::size()
{
	return (pptr() - pbase());
}

void ViewerStream::ViewerStreamBuf::reset()
{
	memset(buf, 0, sizeof(buf));
	setp(buf, buf + FURVIEWER_MAX_OUTPUT_CHAR);
}

