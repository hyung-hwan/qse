/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _QSE_CMN_BITSET_HPP_
#define _QSE_CMN_BITSET_HPP_


/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

template <int NB>
class Bitset
{
public:
	typedef qse_size_t word_type_t;

	static const int NW = (NB - 1) / (QSE_SIZEOF(word_type_t) * 8) + 1;

	Bitset() { this->empty (); }

	bool operator== (const Bitset<NB>& bs) const
	{
		// [NOTE] 
		//   if you happen to have different garbage at the unused part
		//   of the last word, this equality check may return a wrong result.
		for (int i = 0; i < NW; i++)
		{
			if (this->_w[i] != bs._w[i]) return false;
		}
		return true;
	}

	bool operator!= (const Bitset<NB>& bs) const
	{
		return !this->operator==(bs);
	}

	int get_word_index (int bit) const
	{
		return bit / (QSE_SIZEOF(word_type_t) * 8);
	}

	word_type_t get_word_mask (int bit) const
	{
		return ((word_type_t)1) << (bit % (QSE_SIZEOF(word_type_t) * 8));
	}

	void set (int bit)
	{
		this->_w[this->get_word_index(bit)] |= this->get_word_mask(bit);
	}

	void unset (int bit)
	{
		this->_w[this->get_word_index(bit)] &= ~this->get_word_mask(bit);
	}

	void set (const Bitset<NB>& bs)
	{
		// mask on all bits set in bs.
		for (int i = 0; i < NW; i++)
		{
			this->_w[i] |= bs._w[i];
		}
	}

	void unset (const Bitset<NB>& bs)
	{
		// mask off all bits set in bs.
		for (int i = 0; i < NW; i++)
		{
			this->_w[i] &= ~bs._w[i];
		}
	}

	bool isSet (int bit) const
	{
		return (this->_w[this->get_word_index(bit)] & this->get_word_mask(bit));
	}

	bool isEmpty () const
	{
		for (int i = 0; i < NW; i++) 
		{
			if (this->_w[i]) return false;
		}
		return true;
	}

	void empty ()
	{
		for (int i = 0; i < NW; i++) this->_w[i] = 0;
	}

	void fill ()
	{
		for (int i = 0; i < NW; i++) this->_w[i] = ~(word_type_t)0;
	}

protected:
	word_type_t _w[NW];
};

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
