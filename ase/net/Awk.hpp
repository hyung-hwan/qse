/*
 * $Id: Awk.hpp,v 1.5 2007/07/18 11:12:34 bacon Exp $
 */

#pragma once

#include <ase/awk/Awk.hpp>

using namespace System;

namespace ASE
{
	namespace Net
	{

		public ref class Awk abstract
		{
		public:
			/*
			ref class Source
			{
			public:
				enum class MODE
				{
					READ = ASE::Awk::Source::READ,
					WRITE = ASE::Awk::Source::WRITE
				};

				property MODE^ Mode
				{
					MODE^ get () { return this->mode; }
					void set (MODE^ mode) { this->mode = mode; }
				};

			private:
				MODE^ mode;
			};*/

			ref class Extio
			{
			public:
				Extio (): handle (nullptr)
				{
				}

				property Object^ Handle
				{
					Object^ get () { return this->handle; }
					void set (Object^ handle) { this->handle = handle; }
				}

			private:
				Object^ handle;
			};

			ref class Pipe: public Extio
			{
			public:
				enum class MODE
				{
					READ = ASE::Awk::Pipe::READ,
					WRITE = ASE::Awk::Pipe::WRITE
				};

				property MODE^ Mode
				{
					MODE^ get () { return this->mode; }
					void set (MODE^ mode) { this->mode = mode; }
				};

			private:
				MODE^ mode;
			};

			ref class File: public Extio
			{
			public:
				enum class MODE
				{
					READ = ASE::Awk::File::READ,
					WRITE = ASE::Awk::File::WRITE,
					APPEND = ASE::Awk::File::APPEND
				};

				property MODE^ Mode
				{
					MODE^ get () { return this->mode; }
					void set (MODE^ mode) { this->mode = mode; }
				};

			private:
				MODE^ mode;
			};

			ref class Console: public Extio
			{
			public:
				enum class MODE
				{
					READ = ASE::Awk::Console::READ,
					WRITE = ASE::Awk::Console::WRITE
				};

				property MODE^ Mode
				{
					MODE^ get () { return this->mode; }
					void set (MODE^ mode) { this->mode = mode; }
				};

			private:
				MODE^ mode;
			};

			typedef ASE::Awk::char_t char_t;

			Awk ();
			virtual ~Awk ();

			//bool Open ();
			void Close ();

			bool Parse ();
			bool Run ();

			delegate System::Object^ FunctionHandler (array<System::Object^>^ args);

			bool AddFunction (System::String^ name, int minArgs, int maxArgs, FunctionHandler^ handler);
			bool DeleteFunction (System::String^ name);

			property System::IO::Stream^ SourceInputStream
			{
				System::IO::Stream^ get () 
				{ 
					return this->sourceInputStream; 
				}

				void set (System::IO::Stream^ stream) 
				{ 
					this->sourceInputStream = stream; 
				}
			}

			property System::IO::Stream^ SourceOutputStream
			{
				System::IO::Stream^ get () 
				{ 
					return this->sourceOutputStream; 
				}

				void set (System::IO::Stream^ stream) 
				{ 
					this->sourceOutputStream = stream; 
				}
			}

			delegate int OpenPipe (Pipe^ pipe);
			delegate int ClosePipe (Pipe^ pipe);
			delegate int ReadPipe (Pipe^ pipe, cli::array<char_t>^ buf, int len);
			delegate int WritePipe (Pipe^ pipe, cli::array<char_t>^ buf, int len);
			delegate int FlushPipe (Pipe^ pipe);

			delegate int OpenFile (File^ file);
			delegate int CloseFile (File^ file);
			delegate int ReadFile (File^ file, cli::array<char_t>^ buf, int len);
			delegate int WriteFile (File^ file, cli::array<char_t>^ buf, int len);
			delegate int FlushFile (File^ file);

			delegate int OpenConsole (Console^ console);
			delegate int CloseConsole (Console^ console);
			delegate int ReadConsole (Console^ console, cli::array<char_t>^ buf, int len);
			delegate int WriteConsole (Console^ console, cli::array<char_t>^ buf, int len);
			delegate int FlushConsole (Console^ console);
			delegate int NextConsole (Console^ console);

			event OpenPipe^ OpenPipeHandler;
			event ClosePipe^ ClosePipeHandler;
			event ReadPipe^ ReadPipeHandler;
			event WritePipe^ WritePipeHandler;
			event FlushPipe^ FlushPipeHandler;

			event OpenFile^ OpenFileHandler;
			event CloseFile^ CloseFileHandler;
			event ReadFile^ ReadFileHandler;
			event WriteFile^ WriteFileHandler;
			event FlushFile^ FlushFileHandler;

			event OpenConsole^ OpenConsoleHandler;
			event CloseConsole^ CloseConsoleHandler;
			event ReadConsole^ ReadConsoleHandler;
			event WriteConsole^ WriteConsoleHandler;
			event FlushConsole^ FlushConsoleHandler;
			event NextConsole^ NextConsoleHandler;

		protected:
			ASE::Awk* awk;

			System::IO::Stream^ sourceInputStream;
			System::IO::Stream^ sourceOutputStream;

		public protected:
			int DispatchFunction (System::String^ name);

			int FireOpenFile (File^ file);
			int FireCloseFile (File^ file);
			int FireReadFile (File^ file, cli::array<char_t>^ buf, int len);
			int FireWriteFile (File^ file, cli::array<char_t>^ buf, int len);
			int FireFlushFile (File^ file);

			int FireOpenPipe (Pipe^ pipe);
			int FireClosePipe (Pipe^ pipe);
			int FireReadPipe (Pipe^ pipe, cli::array<char_t>^ buf, int len);
			int FireWritePipe (Pipe^ pipe, cli::array<char_t>^ buf, int len);
			int FireFlushPipe (Pipe^ pipe);

			int FireOpenConsole (Console^ console);
			int FireCloseConsole (Console^ console);
			int FireReadConsole (Console^ console, cli::array<char_t>^ buf, int len);
			int FireWriteConsole (Console^ console, cli::array<char_t>^ buf, int len);
			int FireFlushConsole (Console^ console);
			int FireNextConsole (Console^ console);
		};

	}
}
