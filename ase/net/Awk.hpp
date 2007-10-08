/*
 * $Id: Awk.hpp,v 1.34 2007/10/07 15:27:39 bacon Exp $
 *
 * {License}
 */

#pragma once

#include <ase/awk/Awk.hpp>
#include <vcclr.h>

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
			typedef ASE::Awk::ssize_t ssize_t;
			typedef ASE::Awk::cint_t cint_t;
			typedef ASE::Awk::bool_t bool_t;

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

				/*
				bool GetIndexedLong (System::String^ idx, 
					[System::Runtime::InteropServices::Out] long_t% v)
				{
					ASE::Awk::Argument x;
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					if (arg.getIndexed (ip, idx->Length, x) == -1) return false;
					v = x.toInt ();
					return true;
				}

				bool GetIndexedReal (System::String^ idx,
					[System::Runtime::InteropServices::Out] real_t% v) 
				{
					ASE::Awk::Argument x;
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					if (arg.getIndexed (ip, idx->Length, x) == -1) return false;
					v = x.toReal ();
					return true;
				}

				bool GetIndexedString (System::String^ idx,
					[System::Runtime::InteropServices::Out] System::String^% v) 
				{
					ASE::Awk::Argument x;
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					if (arg.getIndexed (ip, idx->Length, x) == -1) return false;

					size_t len;
					const char_t* s = arg.toStr(&len);
					v = gcnew System::String (s, 0, len);
					return true;
				}*/

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
				bool Set (System::String^ v)
				{
					cli::pin_ptr<const char_t> nptr = PtrToStringChars(v);
					return ret.set (nptr, v->Length) == 0;
				}

				bool Set (long_t v)
				{
					return ret.set (v) == 0;
				}

				bool Set (real_t v)
				{
					return ret.set (v) == 0;
				}

				bool Set (System::Single^ v)
				{
					return ret.set ((real_t)(float)v) == 0;
				}

				bool Set (System::Double^ v)
				{
					return ret.set ((real_t)(double)v) == 0;
				}

				bool Set (System::SByte^ v)
				{
					return ret.set ((long_t)(__int8)v) == 0;
				}

				bool Set (System::Int16^ v)
				{
					return ret.set ((long_t)(__int16)v) == 0;
				}
				
				bool Set (System::Int32^ v)
				{
					return ret.set ((long_t)(__int32)v) == 0;
				}

				bool Set (System::Int64^ v)
				{
					return ret.set ((long_t)(__int64)v) == 0;
				}

				bool Set (System::Byte^ v)
				{
					return ret.set ((long_t)(unsigned __int8)v) == 0;
				}

				bool Set (System::UInt16^ v)
				{
					return ret.set ((long_t)(unsigned __int16)v) == 0;
				}
				
				bool Set (System::UInt32^ v)
				{
					return ret.set ((long_t)(unsigned __int32)v) == 0;
				}

				bool Set (System::UInt64^ v)
				{
					return ret.set ((long_t)(unsigned __int64)v) == 0;
				}

				bool IsIndexed ()
				{
					return ret.isIndexed ();
				}

				bool SetIndexed (System::String^ idx, System::String^ v)
				{
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					cli::pin_ptr<const char_t> vp = PtrToStringChars(v);
					return ret.setIndexed (ip, idx->Length, vp, v->Length) == 0;
				}

				bool SetIndexed (System::String^ idx, long_t v)
				{
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					return ret.setIndexed (ip, idx->Length, v) == 0;
				}

				bool SetIndexed (System::String^ idx, real_t v)
				{
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					return ret.setIndexed (ip, idx->Length, v) == 0;
				}

				bool SetIndexed (System::String^ idx, System::Single^ v)
				{
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					return ret.setIndexed (ip, idx->Length, (real_t)(float)v) == 0;
				}

				bool SetIndexed (System::String^ idx, System::Double^ v)
				{
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					return ret.setIndexed (ip, idx->Length, (real_t)(double)v) == 0;
				}

				bool SetIndexed (System::String^ idx, System::SByte^ v)
				{
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					return ret.setIndexed (ip, idx->Length, (long_t)(__int8)v) == 0;
				}

				bool SetIndexed (System::String^ idx, System::Int16^ v)
				{
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					return ret.setIndexed (ip, idx->Length, (long_t)(__int16)v) == 0;
				}
			
				bool SetIndexed (System::String^ idx, System::Int32^ v)
				{
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					return ret.setIndexed (ip, idx->Length, (long_t)(__int32)v) == 0;
				}

				bool SetIndexed (System::String^ idx, System::Int64^ v)
				{
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					return ret.setIndexed (ip, idx->Length, (long_t)(__int64)v) == 0;
				}

				bool SetIndexed (System::String^ idx, System::Byte^ v)
				{
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					return ret.setIndexed (ip, idx->Length, (long_t)(unsigned __int8)v) == 0;
				}

				bool SetIndexed (System::String^ idx, System::UInt16^ v)
				{
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					return ret.setIndexed (ip, idx->Length, (long_t)(unsigned __int16)v) == 0;
				}
			
				bool SetIndexed (System::String^ idx, System::UInt32^ v)
				{
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					return ret.setIndexed (ip, idx->Length, (long_t)(unsigned __int32)v) == 0;
				}

				bool SetIndexed (System::String^ idx, System::UInt64^ v)
				{
					cli::pin_ptr<const char_t> ip = PtrToStringChars(idx);
					return ret.setIndexed (ip, idx->Length, (long_t)(unsigned __int64)v) == 0;
				}

			protected:
				ASE::Awk::Return& ret;
			};

			ref class Context
			{
			public protected:
				Context (Awk^ owner, const ASE::Awk::Run& run): owner (owner), run ((ASE::Awk::Run&)run)
				{
				}

			public:
				property Awk^ Owner
				{
					Awk^ get () { return this->owner; }
				}

				bool Stop ()
				{
					return run.stop () == 0;
				}

				/* TODO:
				 * GetError
				 * SetError 
				 */

				bool SetGlobal (int id, System::String^ v)
				{
					cli::pin_ptr<const char_t> nptr = PtrToStringChars(v);
					return run.setGlobal (id, nptr, v->Length) == 0;
				}

				bool SetGlobal (int id, long_t v)
				{
					return run.setGlobal (id, v) == 0;
				}
				bool SetGlobal (int id, System::SByte^ v)
				{
					return run.setGlobal (id, (long_t)(__int8)v) == 0;
				}
				bool SetGlobal (int id, System::Int16^ v)
				{
					return run.setGlobal (id, (long_t)(__int16)v) == 0;
				}
				bool SetGlobal (int id, System::Int32^ v)
				{
					return run.setGlobal (id, (long_t)(__int32)v) == 0;
				}
				bool SetGlobal (int id, System::Int64^ v)
				{
					return run.setGlobal (id, (long_t)(__int64)v) == 0;
				}
				bool SetGlobal (int id, System::Byte^ v)
				{
					return run.setGlobal (id, (long_t)(unsigned __int8)v) == 0;
				}
				bool SetGlobal (int id, System::UInt16^ v)
				{
					return run.setGlobal (id, (long_t)(unsigned __int16)v) == 0;
				}
				bool SetGlobal (int id, System::UInt32^ v)
				{
					return run.setGlobal (id, (long_t)(unsigned __int32)v) == 0;
				}
				bool SetGlobal (int id, System::UInt64^ v)
				{
					return run.setGlobal (id, (long_t)(unsigned __int64)v) == 0;
				}

				bool SetGlobal (int id, real_t v)
				{
					return run.setGlobal (id, v) == 0;
				}
				bool SetGlobal (int id, System::Single^ v)
				{
					return run.setGlobal (id, (real_t)(float)v) == 0;
				}
				bool SetGlobal (int id, System::Double^ v)
				{
					return run.setGlobal (id, (real_t)(double)v) == 0;
				}

				/*
				bool GetGlobal (int id, [System::Runtime::InteropServices::Out] Argument^ v)
				{
					return run.getGlobal (id, v->placeHolder) == 0;
				}
				*/

			protected:
				Awk^ owner;
				ASE::Awk::Run& run;
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
	
			[System::Flags] enum class OPTION: int
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
				STRBASEONE = ASE::Awk::OPT_BASEONE,
				STRIPSPACES = ASE::Awk::OPT_STRIPSPACES,
				NEXTOFILE = ASE::Awk::OPT_NEXTOFILE,
				CRLF = ASE::Awk::OPT_CRLF,
				ARGSTOMAIN = ASE::Awk::OPT_ARGSTOMAIN,
				RESET = ASE::Awk::OPT_RESET,
				MAPTOVAR = ASE::Awk::OPT_MAPTOVAR
			};

			enum class DEPTH: int
			{
				BLOCK_PARSE = ASE::Awk::DEPTH_BLOCK_PARSE,
				BLOCK_RUN   = ASE::Awk::DEPTH_BLOCK_RUN,
				EXPR_PARSE  = ASE::Awk::DEPTH_EXPR_PARSE,
				EXPR_RUN    = ASE::Awk::DEPTH_EXPR_RUN,
				REX_BUILD   = ASE::Awk::DEPTH_REX_BUILD,
				REX_MATCH   = ASE::Awk::DEPTH_REX_MATCH
			};

			// generated by generrcode-net.awk
			enum class ERROR: int
			{
				NOERR = ASE::Awk::ERR_NOERR,
				INVAL = ASE::Awk::ERR_INVAL,
				NOMEM = ASE::Awk::ERR_NOMEM,
				NOSUP = ASE::Awk::ERR_NOSUP,
				NOPER = ASE::Awk::ERR_NOPER,
				NODEV = ASE::Awk::ERR_NODEV,
				NOSPC = ASE::Awk::ERR_NOSPC,
				MFILE = ASE::Awk::ERR_MFILE,
				MLINK = ASE::Awk::ERR_MLINK,
				AGAIN = ASE::Awk::ERR_AGAIN,
				NOENT = ASE::Awk::ERR_NOENT,
				EXIST = ASE::Awk::ERR_EXIST,
				FTBIG = ASE::Awk::ERR_FTBIG,
				TBUSY = ASE::Awk::ERR_TBUSY,
				ISDIR = ASE::Awk::ERR_ISDIR,
				IOERR = ASE::Awk::ERR_IOERR,
				OPEN = ASE::Awk::ERR_OPEN,
				READ = ASE::Awk::ERR_READ,
				WRITE = ASE::Awk::ERR_WRITE,
				CLOSE = ASE::Awk::ERR_CLOSE,
				INTERN = ASE::Awk::ERR_INTERN,
				RUNTIME = ASE::Awk::ERR_RUNTIME,
				BLKNST = ASE::Awk::ERR_BLKNST,
				EXPRNST = ASE::Awk::ERR_EXPRNST,
				SINOP = ASE::Awk::ERR_SINOP,
				SINCL = ASE::Awk::ERR_SINCL,
				SINRD = ASE::Awk::ERR_SINRD,
				SOUTOP = ASE::Awk::ERR_SOUTOP,
				SOUTCL = ASE::Awk::ERR_SOUTCL,
				SOUTWR = ASE::Awk::ERR_SOUTWR,
				LXCHR = ASE::Awk::ERR_LXCHR,
				LXDIG = ASE::Awk::ERR_LXDIG,
				LXUNG = ASE::Awk::ERR_LXUNG,
				ENDSRC = ASE::Awk::ERR_ENDSRC,
				ENDCMT = ASE::Awk::ERR_ENDCMT,
				ENDSTR = ASE::Awk::ERR_ENDSTR,
				ENDREX = ASE::Awk::ERR_ENDREX,
				LBRACE = ASE::Awk::ERR_LBRACE,
				LPAREN = ASE::Awk::ERR_LPAREN,
				RPAREN = ASE::Awk::ERR_RPAREN,
				RBRACK = ASE::Awk::ERR_RBRACK,
				COMMA = ASE::Awk::ERR_COMMA,
				SCOLON = ASE::Awk::ERR_SCOLON,
				COLON = ASE::Awk::ERR_COLON,
				STMEND = ASE::Awk::ERR_STMEND,
				IN = ASE::Awk::ERR_IN,
				NOTVAR = ASE::Awk::ERR_NOTVAR,
				EXPRES = ASE::Awk::ERR_EXPRES,
				WHILE = ASE::Awk::ERR_WHILE,
				ASSIGN = ASE::Awk::ERR_ASSIGN,
				IDENT = ASE::Awk::ERR_IDENT,
				FNNAME = ASE::Awk::ERR_FNNAME,
				BLKBEG = ASE::Awk::ERR_BLKBEG,
				BLKEND = ASE::Awk::ERR_BLKEND,
				DUPBEG = ASE::Awk::ERR_DUPBEG,
				DUPEND = ASE::Awk::ERR_DUPEND,
				BFNRED = ASE::Awk::ERR_BFNRED,
				AFNRED = ASE::Awk::ERR_AFNRED,
				GBLRED = ASE::Awk::ERR_GBLRED,
				PARRED = ASE::Awk::ERR_PARRED,
				DUPPAR = ASE::Awk::ERR_DUPPAR,
				DUPGBL = ASE::Awk::ERR_DUPGBL,
				DUPLCL = ASE::Awk::ERR_DUPLCL,
				BADPAR = ASE::Awk::ERR_BADPAR,
				BADVAR = ASE::Awk::ERR_BADVAR,
				UNDEF = ASE::Awk::ERR_UNDEF,
				LVALUE = ASE::Awk::ERR_LVALUE,
				GBLTM = ASE::Awk::ERR_GBLTM,
				LCLTM = ASE::Awk::ERR_LCLTM,
				PARTM = ASE::Awk::ERR_PARTM,
				DELETE = ASE::Awk::ERR_DELETE,
				BREAK = ASE::Awk::ERR_BREAK,
				CONTINUE = ASE::Awk::ERR_CONTINUE,
				NEXTBEG = ASE::Awk::ERR_NEXTBEG,
				NEXTEND = ASE::Awk::ERR_NEXTEND,
				NEXTFBEG = ASE::Awk::ERR_NEXTFBEG,
				NEXTFEND = ASE::Awk::ERR_NEXTFEND,
				PRINTFARG = ASE::Awk::ERR_PRINTFARG,
				PREPST = ASE::Awk::ERR_PREPST,
				GLNCPS = ASE::Awk::ERR_GLNCPS,
				DIVBY0 = ASE::Awk::ERR_DIVBY0,
				OPERAND = ASE::Awk::ERR_OPERAND,
				POSIDX = ASE::Awk::ERR_POSIDX,
				ARGTF = ASE::Awk::ERR_ARGTF,
				ARGTM = ASE::Awk::ERR_ARGTM,
				FNNONE = ASE::Awk::ERR_FNNONE,
				NOTIDX = ASE::Awk::ERR_NOTIDX,
				NOTDEL = ASE::Awk::ERR_NOTDEL,
				NOTMAP = ASE::Awk::ERR_NOTMAP,
				NOTMAPIN = ASE::Awk::ERR_NOTMAPIN,
				NOTMAPNILIN = ASE::Awk::ERR_NOTMAPNILIN,
				NOTREF = ASE::Awk::ERR_NOTREF,
				NOTASS = ASE::Awk::ERR_NOTASS,
				IDXVALASSMAP = ASE::Awk::ERR_IDXVALASSMAP,
				POSVALASSMAP = ASE::Awk::ERR_POSVALASSMAP,
				MAPTOSCALAR = ASE::Awk::ERR_MAPTOSCALAR,
				SCALARTOMAP = ASE::Awk::ERR_SCALARTOMAP,
				MAPNOTALLOWED = ASE::Awk::ERR_MAPNOTALLOWED,
				VALTYPE = ASE::Awk::ERR_VALTYPE,
				RDELETE = ASE::Awk::ERR_RDELETE,
				RNEXTBEG = ASE::Awk::ERR_RNEXTBEG,
				RNEXTEND = ASE::Awk::ERR_RNEXTEND,
				RNEXTFBEG = ASE::Awk::ERR_RNEXTFBEG,
				RNEXTFEND = ASE::Awk::ERR_RNEXTFEND,
				BFNUSER = ASE::Awk::ERR_BFNUSER,
				BFNIMPL = ASE::Awk::ERR_BFNIMPL,
				IOUSER = ASE::Awk::ERR_IOUSER,
				IONONE = ASE::Awk::ERR_IONONE,
				IOIMPL = ASE::Awk::ERR_IOIMPL,
				IONMEM = ASE::Awk::ERR_IONMEM,
				IONMNL = ASE::Awk::ERR_IONMNL,
				FMTARG = ASE::Awk::ERR_FMTARG,
				FMTCNV = ASE::Awk::ERR_FMTCNV,
				CONVFMTCHR = ASE::Awk::ERR_CONVFMTCHR,
				OFMTCHR = ASE::Awk::ERR_OFMTCHR,
				REXRECUR = ASE::Awk::ERR_REXRECUR,
				REXRPAREN = ASE::Awk::ERR_REXRPAREN,
				REXRBRACKET = ASE::Awk::ERR_REXRBRACKET,
				REXRBRACE = ASE::Awk::ERR_REXRBRACE,
				REXUNBALPAR = ASE::Awk::ERR_REXUNBALPAR,
				REXCOLON = ASE::Awk::ERR_REXCOLON,
				REXCRANGE = ASE::Awk::ERR_REXCRANGE,
				REXCCLASS = ASE::Awk::ERR_REXCCLASS,
				REXBRANGE = ASE::Awk::ERR_REXBRANGE,
				REXEND = ASE::Awk::ERR_REXEND,
				REXGARBAGE = ASE::Awk::ERR_REXGARBAGE
			};
			// end of enum class ERROR

			typedef ASE::Awk::char_t char_t;

			Awk ();
			!Awk ();
			virtual ~Awk ();

			virtual void Close ();
			virtual bool Parse ();
			virtual bool Run ();
			virtual bool Run (System::String^ entryPoint, cli::array<System::String^>^ args);
			virtual void Stop ();

			delegate void RunStartHandler (Context^ ctx);
			delegate void RunEndHandler  (Context^ ctx);
			delegate void RunReturnHandler (Context^ ctx);
			delegate void RunStatementHandler (Context^ ctx);

			/*event*/ RunStartHandler^ OnRunStart;
			/*event*/ RunEndHandler^ OnRunEnd;
			/*event*/ RunReturnHandler^ OnRunReturn;
			/*event*/ RunStatementHandler^ OnRunStatement;

			virtual bool AddGlobal (
				System::String^ name, 
				[System::Runtime::InteropServices::Out] int% id);
			virtual bool DeleteGlobal (System::String^ name);

			delegate bool FunctionHandler (
				System::String^ name, cli::array<Argument^>^ args, Return^ ret);

			virtual bool AddFunction (
				System::String^ name, int minArgs, 
				int maxArgs, FunctionHandler^ handler);
			virtual bool DeleteFunction (System::String^ name);

			virtual bool SetWord (System::String^ ow, System::String^ nw);
			virtual bool UnsetWord (System::String^ ow);
			virtual bool UnsetAllWords ();

			virtual bool SetMaxDepth (DEPTH id, size_t depth);
			virtual bool GetMaxDepth (DEPTH id, size_t* depth);

			virtual bool SetErrorString (ERROR num, System::String^ str);
			
			property OPTION Option
			{
				OPTION get ();
				void set (OPTION opt);
			}

			property System::String^ ErrorMessage
			{
				System::String^ get () { return this->errMsg; }
			}

			property ERROR ErrorCode
			{
				ERROR get () { return this->errCode; }
			}

			property unsigned int ErrorLine
			{
				unsigned int get () { return this->errLine; }
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
			bool Awk::DispatchFunction (
				ASE::Awk::Run& run, ASE::Awk::Return& ret, 
				const ASE::Awk::Argument* args, size_t nargs, 
				const char_t* name, size_t len);

		public protected:
			System::String^ errMsg;
			unsigned int errLine;
			ERROR errCode;

			void setError (ERROR num);
			void retrieveError ();
			bool runErrorReported; // only used if the run-callback is activated.
		};

	}
}
