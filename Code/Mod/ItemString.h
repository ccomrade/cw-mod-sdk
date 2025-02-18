////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2006.
// -------------------------------------------------------------------------
//  File name:   ItemString.h
//  Version:     v1.00
//  Created:     20/02/2007 by AlexL
//  Compilers:   Visual Studio.NET
//  Description: Special version of CCryName
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __ITEMSTRING_H__
#define __ITEMSTRING_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <unordered_map>

#include <ISystem.h>
#include <StlUtils.h>

namespace SharedString
{
	struct NameHash
	{
		static unsigned char ToLowerFast(unsigned char ch)
		{
			return ch | ((ch >= 'A' && ch <= 'Z') << 5);
		}

		size_t operator()(const char* key) const
		{
			unsigned int hash = 0;
			for (; *key; ++key)
			{
				hash = (5 * hash) + ToLowerFast(static_cast<unsigned char>(*key));
			}

			return hash;
		}
	};

	struct NameEqual
	{
		bool operator()(const char* a, const char* b) const
		{
			return strcmp(a, b) == 0;
		}
	};

	// Name entry header, immediately after this header in memory starts actual string data.
	struct SNameEntry
	{
		int nRefCount;		// Reference count of this string.
		int nLength;			// Current length of string.
		int nAllocSize;		// Size of memory allocated at the end of this class.

		// Here in memory starts character buffer of size nAllocSize.
		//char data[nAllocSize]

		char* GetStr()  { return (char*)(this+1); }
		void  AddRef()  { ++nRefCount; };
		int   Release() { return --nRefCount; };
	};

	//////////////////////////////////////////////////////////////////////////
	class CNameTable
	{
	public:
		CNameTable()
			: m_nameMap(1024)
		{}

		~CNameTable()
		{
		}

		// Only finds an existing name table entry, return 0 if not found.
		SNameEntry* FindEntry( const char *str )
		{
			SNameEntry *pEntry = stl::find_in_map( m_nameMap,str,0 );
			return pEntry;
		}

		// Finds an existing name table entry, or creates a new one if not found.
		SNameEntry* GetEntry( const char *str )
		{
			SNameEntry *pEntry = stl::find_in_map( m_nameMap,str,0 );
			if (!pEntry)
			{
				// Create a new entry.
				unsigned int nLen = strlen(str);
				unsigned int allocLen = sizeof(SNameEntry) + (nLen+1)*sizeof(char);
				pEntry = (SNameEntry*)malloc( allocLen );
				pEntry->nRefCount = 0;
				pEntry->nLength = nLen;
				pEntry->nAllocSize = allocLen;
				// Copy string to the end of name entry.
				memcpy( pEntry->GetStr(),str,nLen+1 );

				// put in map.
				m_nameMap[pEntry->GetStr()] = pEntry;
			}
			return pEntry;
		}

		// Release existing name table entry.
		void Release( SNameEntry *pEntry )
		{
/*
			assert(pEntry);
			m_nameMap.erase( pEntry->GetStr() );
			free(pEntry);
*/
		}

		void Dump()
		{
			CryLogAlways("NameTable: %u entries", static_cast<unsigned int>(m_nameMap.size()));
			for (const auto& [name, pEntry] : m_nameMap)
			{
				CryLogAlways("'%s'", name);
			}
		}

	private:
		std::unordered_map<const char*, SNameEntry*, NameHash, NameEqual> m_nameMap;
	};

	///////////////////////////////////////////////////////////////////////////////
	// Class CSharedString.
	//////////////////////////////////////////////////////////////////////////
	class	CSharedString
	{
	public:
		CSharedString();
		CSharedString( const CSharedString& n );
		CSharedString( const char *s );
		CSharedString( const char *s,bool bOnlyFind );
		~CSharedString();

		CSharedString& operator=( const CSharedString& n );
		CSharedString& operator=( const char *s );

		//! cast to C string operator.
		operator const char*() const { return (m_str) ? m_str: ""; }
		operator bool() const { return m_str != 0; }
		bool operator!() const { return m_str == 0; }

		bool	operator==( const CSharedString &n ) const;
		bool	operator!=( const CSharedString &n ) const;

		bool	operator==( const char *s ) const;
		bool	operator!=( const char *s ) const;

		bool	operator<( const CSharedString &n ) const;
		bool	operator>( const CSharedString &n ) const;

		bool	empty() const { return length() == 0; }
		void	reset()	{	_release(m_str);	m_str = 0; }
		void  clear() { reset(); }
		const	char*	c_str() const { return (m_str) ? m_str: ""; }
		int	length() const { return _length(); };

		static bool find( const char *str ) { return GetNameTable()->FindEntry(str) != 0; }
		static const char *create( const char *str )
		{
			CSharedString name = CSharedString(str);
			name._addref(name.c_str());
			return name.c_str();
		}

		static void DumpNameTable()
		{
			GetNameTable()->Dump();
		}

	private:
		static CNameTable* GetNameTable()
		{
			static CNameTable table;
			return &table;
		}


		SNameEntry* _entry( const char *pBuffer ) const { assert(pBuffer); return ((SNameEntry*)pBuffer)-1; }
		int  _length() const { return (m_str) ? _entry(m_str)->nLength : 0; };
		void _addref( const char *pBuffer ) { if (pBuffer) _entry(pBuffer)->AddRef(); }
		void _release( const char *pBuffer )
		{
			if (pBuffer && _entry(pBuffer)->Release() <= 0)
			{
				if (CNameTable* pNT = GetNameTable())
					pNT->Release(_entry(pBuffer));
			}
		}

		const char *m_str;
	};

	//////////////////////////////////////////////////////////////////////////
	inline CSharedString::CSharedString()
	{
		m_str = 0;
	}

	//////////////////////////////////////////////////////////////////////////
	inline CSharedString::CSharedString( const CSharedString& n )
	{
		_addref( n.m_str );
		m_str = n.m_str;
	}

	//////////////////////////////////////////////////////////////////////////
	inline CSharedString::CSharedString( const char *s )
	{
		m_str = 0;
		*this = s;
	}

	//////////////////////////////////////////////////////////////////////////
	inline CSharedString::CSharedString( const char *s,bool bOnlyFind )
	{
		assert(s);
		m_str = 0;
		const char *pBuf = 0;
		if (*s) // if not empty
		{
			SNameEntry *pNameEntry = GetNameTable()->FindEntry(s);
			if (pNameEntry)
			{
				m_str = pNameEntry->GetStr();
				_addref(m_str);
			}
		}
	}

	inline CSharedString::~CSharedString()
	{
		_release(m_str);
	}

	//////////////////////////////////////////////////////////////////////////
	inline CSharedString&	CSharedString::operator=( const CSharedString &n )
	{
		if (m_str != n.m_str)
		{
			_release(m_str);
			m_str = n.m_str;
			_addref(m_str);
		}
		return *this;
	}

	//////////////////////////////////////////////////////////////////////////
	inline CSharedString&	CSharedString::operator=( const char *s )
	{
		//assert(s); // AlexL: we currenly allow 0 assignment because Items currently do this a lot
		const char *pBuf = 0;
		if (s && *s) // if not empty
		{
			pBuf = GetNameTable()->GetEntry(s)->GetStr();
		}
		else if (s == 0)
		{
			; // debugging here
		}
		if (m_str != pBuf)
		{
			_release(m_str);
			m_str = pBuf;
			_addref(m_str);
		}
		return *this;
	}


	//////////////////////////////////////////////////////////////////////////
	inline bool	CSharedString::operator==( const CSharedString &n ) const {
		return m_str == n.m_str;
	}

	inline bool	CSharedString::operator!=( const CSharedString &n ) const {
		return !(*this == n);
	}

	inline bool	CSharedString::operator==( const char* str ) const {
		return m_str && strcmp(m_str,str) == 0;
	}

	inline bool	CSharedString::operator!=( const char* str ) const {
		if (!m_str)
			return true;
		return strcmp(m_str,str) != 0;
	}

	inline bool	CSharedString::operator<( const CSharedString &n ) const {
		return m_str < n.m_str;
	}

	inline bool	CSharedString::operator>( const CSharedString &n ) const {
		return m_str > n.m_str;
	}

}; // _ItemString


#define ITEM_USE_SHAREDSTRING

#ifdef ITEM_USE_SHAREDSTRING
typedef SharedString::CSharedString ItemString;
#define CONST_TEMPITEM_STRING(a) a
#else
typedef string ItemString;
#define CONST_TEMPITEM_STRING(a) CONST_TEMP_STRING(a)
#endif

#endif
