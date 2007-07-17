/*
 * $Id: Awk.hpp,v 1.3 2007/07/16 11:12:12 bacon Exp $
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

			bool Open ();
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
			
		protected:
			ASE::Awk* awk;

			System::IO::Stream^ sourceInputStream;
			System::IO::Stream^ sourceOutputStream;

		public protected:
			int Awk::DispatchFunction (System::String^ name);
		};

	}
}
