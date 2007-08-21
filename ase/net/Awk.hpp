/*
 * $Id: Awk.hpp,v 1.14 2007/08/20 14:19:58 bacon Exp $
 */

#pragma once

#include <ase/awk/Awk.hpp>
#include <vcclr.h>

using namespace System;

namespace ASE
{
	class MojoAwk;

	namespace Net
	{
		public ref class Awk abstract
		{
		public:
			typedef ASE::Awk::long_t long_t;
			typedef ASE::Awk::real_t real_t;
			typedef ASE::Awk::char_t char_t;
			typedef ASE::Awk::size_t size_t;

			ref class Argument
			{
			public protected:
				Argument (const ASE::Awk::Argument& arg): arg(arg)
				{
				}

			public:
				property long_t LongValue 
				{
					long_t get () { return arg.toInt(); }
				}

				property real_t RealValue
				{
					real_t get () { return arg.toReal(); }
				}

				property System::String^ StringValue
				{ 
					System::String^ get ()
					{
						size_t len;
						const char_t* s = arg.toStr(&len);
						return gcnew System::String (s, 0, len);
					}
				}

			protected:
				const ASE::Awk::Argument& arg;
			};

			ref class Return
			{
			public protected:
				Return (ASE::Awk::Return& ret): ret (ret)
				{
				}

			public:
				property System::String^ StringValue
				{
					void set (System::String^ v)
					{
						cli::pin_ptr<const char_t> nptr = PtrToStringChars(v);
						ret.set (nptr, v->Length);
					}
				}

				property long_t LongValue
				{
					void set (long_t v)
					{
						ret.set (v);
					}
				}

				property real_t RealValue
				{
					void set (real_t v)
					{
						ret.set (v);
					}
				}

				property System::Single^ SingleValue
				{
					void set (System::Single^ v)
					{
						ret.set ((real_t)(float)v);
					}
				}

				property System::Double^ DoubleValue
				{
					void set (System::Double^ v)
					{
						ret.set ((real_t)(double)v);
					}
				}

				property System::SByte^ SByteValue
				{
					void set (System::SByte^ v)
					{
						ret.set ((long_t)(__int8)v);
					}
				}

				property System::Int16^ Int16Value
				{
					void set (System::Int16^ v)
					{
						ret.set ((long_t)(__int16)v);
					}
				}
				
				property System::Int32^ Int32Value
				{
					void set (System::Int32^ v)
					{
						ret.set ((long_t)(__int32)v);
					}
				}

				property System::Int64^ Int64Value
				{
					void set (System::Int64^ v)
					{
						ret.set ((long_t)(__int64)v);
					}
				}

				property System::Byte^ ByteValue
				{
					void set (System::Byte^ v)
					{
						ret.set ((long_t)(unsigned __int8)v);
					}
				}

				property System::UInt16^ UInt16Value
				{
					void set (System::UInt16^ v)
					{
						ret.set ((long_t)(unsigned __int16)v);
					}
				}
				
				property System::UInt32^ UInt32Value
				{
					void set (System::UInt32^ v)
					{
						ret.set ((long_t)(unsigned __int32)v);
					}
				}

				property System::UInt64^ UInt64Value
				{
					void set (System::UInt64^ v)
					{
						ret.set ((long_t)(unsigned __int64)v);
					}
				}

			public:
				ASE::Awk::Return& ret;
			};

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

			
			[Flags] enum class OPTION: int
			{
				NONE = 0,
				IMPLICIT = ASE::Awk::OPT_IMPLICIT,
				EXPLICIT = ASE::Awk::OPT_EXPLICIT,
				UNIQUEFN = ASE::Awk::OPT_UNIQUEFN,
				SHADING = ASE::Awk::OPT_SHADING,
				SHIFT = ASE::Awk::OPT_SHIFT,
				IDIV = ASE::Awk::OPT_IDIV,
				STRCONCAT = ASE::Awk::OPT_STRCONCAT,
				EXTIO = ASE::Awk::OPT_EXTIO,
				COPROC = ASE::Awk::OPT_COPROC,
				BLOCKLESS = ASE::Awk::OPT_BLOCKLESS,
				STRBASEONE = ASE::Awk::OPT_STRBASEONE,
				STRIPSPACES = ASE::Awk::OPT_STRIPSPACES,
				NEXTOFILE = ASE::Awk::OPT_NEXTOFILE,
				CRLF = ASE::Awk::OPT_CRLF,
				ARGSTOMAIN = ASE::Awk::OPT_ARGSTOMAIN
			};

			typedef ASE::Awk::char_t char_t;

			Awk ();
			!Awk ();
			virtual ~Awk ();

			//bool Open ();
			void Close ();

			bool Parse ();
			bool Run ();

			delegate bool FunctionHandler (System::String^ name, array<Argument^>^ args, Return^ ret);

			bool AddFunction (System::String^ name, int minArgs, int maxArgs, FunctionHandler^ handler);
			bool DeleteFunction (System::String^ name);

			property OPTION Option
			{
				OPTION get (); //{ return this->option; }
				void set (OPTION opt); //{ this->option = opt; }
			}

		protected:
			MojoAwk* awk;
			OPTION option;
			System::Collections::Hashtable^ funcs;

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
			bool Awk::DispatchFunction (ASE::Awk::Return* ret, 
				const ASE::Awk::Argument* args, size_t nargs, 
				const char_t* name, size_t len);
		};

	}
}
