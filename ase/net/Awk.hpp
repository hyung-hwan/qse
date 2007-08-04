/*
 * $Id: Awk.hpp,v 1.8 2007/07/29 14:42:33 bacon Exp $
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
			ref class Source
			{
			public:
				enum class MODE
				{
					READ = ASE::Awk::Source::READ,
					WRITE = ASE::Awk::Source::WRITE
				};

				Source (MODE^ mode): handle (nullptr) 
				{
					this->mode = mode;
				}

				property Object^ Handle
				{
					Object^ get () { return this->handle; }
					void set (Object^ handle) { this->handle = handle; }
				}

				property MODE^ Mode
				{
					MODE^ get () { return this->mode; }
				};

			private:
				MODE^ mode;
				Object^ handle;
			};

			ref class Extio
			{
			public:
				Extio (System::String^ name): handle (nullptr)
				{
					this->name = name;
				}

				property Object^ Handle
				{
					Object^ get () { return this->handle; }
					void set (Object^ handle) { this->handle = handle; }
				}

				property System::String^ Name
				{
					System::String^ get () { return this->name; }
				};

			private:
				Object^ handle;
				System::String^ name;
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
				};

				Pipe (System::String^ name, MODE^ mode): Extio (name)
				{
					this->mode = mode;
				}

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
				};

				File (System::String^ name, MODE^ mode): Extio (name)
				{
					this->mode = mode;
				}

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
				};

				Console (System::String^ name, MODE^ mode): Extio (name)
				{
					this->mode = mode;
				}

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

#if 0
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
#endif

		protected:
			ASE::Awk* awk;

#if 0
			System::IO::Stream^ sourceInputStream;
			System::IO::Stream^ sourceOutputStream;
#endif

		public protected:
			// Source
			virtual int OpenSource (Source^ source) = 0;
			virtual int CloseSource (Source^ source) = 0;
			virtual int ReadSource (
				Source^ source, cli::array<char_t>^ buf, int len) = 0;
			virtual int WriteSource (
				Source^ source, cli::array<char_t>^ buf, int len) = 0;
			
			// File
			virtual int OpenFile (File^ file) = 0;
			virtual int CloseFile (File^ file) = 0;
			virtual int ReadFile (
				File^ file, cli::array<char_t>^ buf, int len) = 0;
			virtual int WriteFile (
				File^ file, cli::array<char_t>^ buf, int len) = 0;
			virtual int FlushFile (File^ file) = 0;

			// Pipe
			virtual int OpenPipe (Pipe^ pipe) = 0;
			virtual int ClosePipe (Pipe^ pipe) = 0;
			virtual int ReadPipe (
				Pipe^ pipe, cli::array<char_t>^ buf, int len) = 0;
			virtual int WritePipe (
				Pipe^ pipe, cli::array<char_t>^ buf, int len) = 0;
			virtual int FlushPipe (Pipe^ pipe) = 0;

			// Console
			virtual int OpenConsole (Console^ console) = 0;
			virtual int CloseConsole (Console^ console) = 0;
			virtual int ReadConsole (
				Console^ console, cli::array<char_t>^ buf, int len) = 0;
			virtual int WriteConsole (
				Console^ console, cli::array<char_t>^ buf, int len) = 0;
			virtual int FlushConsole (Console^ console) = 0;
			virtual int NextConsole (Console^ console) = 0;

		public protected:
			int DispatchFunction (System::String^ name);

		};

	}
}
