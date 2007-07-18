/*
 * $Id: Awk.hpp,v 1.4 2007/07/17 09:46:19 bacon Exp $
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
			delegate int ReadPipe (Pipe^ pipe);
			delegate int WritePipe (Pipe^ pipe);
			delegate int FlushPipe (Pipe^ pipe);

			delegate int OpenFile (File^ file);
			delegate int CloseFile (File^ file);
			delegate int ReadFile (File^ file);
			delegate int WriteFile (File^ file);
			delegate int FlushFile (File^ file);

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
			
		protected:
			ASE::Awk* awk;

			System::IO::Stream^ sourceInputStream;
			System::IO::Stream^ sourceOutputStream;

		public protected:
			int DispatchFunction (System::String^ name);

			int FireOpenFile (File^ file);
			int FireCloseFile (File^ file);
			int FireReadFile (File^ file);
			int FireWriteFile (File^ file);
			int FireFlushFile (File^ file);

			int FireOpenPipe (Pipe^ pipe);
			int FireClosePipe (Pipe^ pipe);
			int FireReadPipe (Pipe^ pipe);
			int FireWritePipe (Pipe^ pipe);
			int FireFlushPipe (Pipe^ pipe);
		};

	}
}
