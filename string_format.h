/*****************************************************************************
* Copyright © 2003-2012 Sergey Radionov <rsatom_gmail.com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sergey Radionov aka RSATom nor the
*       names of project contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#if !defined(STRING_FORMAT_H)
#define STRING_FORMAT_H

#include <sstream>

/* usage example ("%_" will be replaced):
*  string_format fmt("%_@%_.%_");
*  fmt<<"vasya.pupkin"<<"gmail"<<"com"; //or fmt%"vasya.pupkin"%"gmail"%"com";
*  std::string str = fmt; //str will contain "vasya.pupkin@gmail.com"
*/

template<class Ch, class Tr=std::char_traits<Ch> >
class basic_string_format
{
public:
	typedef std::basic_string<Ch, Tr>	string_type;

private:
	typedef typename string_type::size_type string_size_type;
	typedef std::basic_ostringstream<Ch, Tr>	ostringstream_type;

public:
	/*
	формат строки:
		"%%" заменяется на "%";
		"%_" является местом под значение;
		если в строке не осталось больше мест под значение,
		но делается попытка добавить еще одно значение,
		то оно просто добавляется в конец.
	*/

	basic_string_format():m_FormatStringPos(0){}

	basic_string_format(const Ch* FormatStr)
		:m_FormatString(FormatStr), m_FormatStringPos(0){push_next();}

	basic_string_format(const string_type& FormatStr)
		:m_FormatString(FormatStr), m_FormatStringPos(0){push_next();}

	//обнуляет результат, но строка формата остатется прежней
	void clear();
	//выставляет новую строку формата и обнуляет результат
	void set_format_str(const Ch* FormatStr);
	void set_format_str(const string_type& FormatStr);

	template<class T>
	basic_string_format& operator<<(const T& x)
	{
		return (*this)%x;
	}

	//позволяет отправлять одно и то же значение несколько раз
	template<class T>
	basic_string_format& operator()(const T& x, unsigned char Count)
	{
		for(;Count>0;--Count) (*this)%x;
		return (*this);
	}

	template<class T>
	basic_string_format& operator()(const T& x)
	{
		return (*this)%x;
	}

	template<class T>
	basic_string_format& operator%(const T& x)
	{
		m_OutStream<<x;
		push_next();
		return *this;
	}

	basic_string_format& operator%(
		std::ios_base& (*pf)(std::ios_base&))
	{
		m_OutStream<<pf;
		return *this;
	}

	basic_string_format& operator%(
		std::basic_ios<Ch, Tr>& (*pf)(std::basic_ios<Ch, Tr>&))
	{
		m_OutStream<<pf;
		return *this;
	}

	basic_string_format& operator%(
		std::basic_ostream<Ch, Tr>& (*pf)(std::basic_ostream<Ch, Tr>&))
	{
		m_OutStream<<pf;
		return *this;
	}

	string_type str()
	{
		push_remainder();
		return m_OutStream.str();
	}

	operator string_type()
	{
		return str();
	}

private:
	void push_next();
	void push_remainder();

private:
	string_type m_FormatString;
	string_size_type m_FormatStringPos;
	ostringstream_type m_OutStream;
};

//обнуляет результат, но строка формата остатется прежней
template<class Ch, class Tr>
void basic_string_format<Ch, Tr>::clear()
{
	m_OutStream.str(string_type());
}

template<class Ch, class Tr>
void basic_string_format<Ch, Tr>::set_format_str(const Ch* FormatStr)
{
	set_format_str(string_type(FormatStr));
}

template<class Ch, class Tr>
void basic_string_format<Ch, Tr>::set_format_str(const string_type& FormatStr)
{
	clear();
	m_FormatString = FormatStr;
	m_FormatStringPos = 0;
	push_next();
}

template<class Ch, class Tr>
void basic_string_format<Ch, Tr>::push_next()
{
	//уже в конце строки формата
	if(m_FormatStringPos==string_type::npos) return;

	const Ch ArgChar  =m_OutStream.widen('%');
	const Ch ArgChar2 =m_OutStream.widen('_');
	const string_size_type FormatLen = m_FormatString.length();

	bool Continue;
	do{
		Continue = false;
		string_size_type NewPos =
		m_FormatString.find(ArgChar, m_FormatStringPos);

		if(NewPos!=string_type::npos){
			char PlaceholderLen=1;//может быть 1 или 2 (в случае "%_")
			if(NewPos+1<FormatLen){
				const Ch Char2 = m_FormatString[NewPos+1];

				if(Char2==ArgChar){// "%%"
					++NewPos;
					Continue=true;
				}
				else if(Char2==ArgChar2) PlaceholderLen=2;// "%_"
				//else //случай с одинарным "%"
			}
			//else //случай с одинарным "%" в конце строки формата

			//сохраняем часть строки между параметрами
			m_OutStream.write(&(m_FormatString.c_str()[m_FormatStringPos]),
			                  NewPos-m_FormatStringPos);

			m_FormatStringPos=NewPos+PlaceholderLen;
		}
		else{
			//сохраняем остаток строки
			push_remainder();
		}
	}while(Continue);
}

template<class Ch, class Tr>
void basic_string_format<Ch, Tr>::push_remainder()
{
	if(m_FormatStringPos!=basic_string_format<Ch, Tr>::string_type::npos){
		//сохраняем остаток строки
		m_OutStream.write(&(m_FormatString.c_str()[m_FormatStringPos]),
		                  m_FormatString.length()-m_FormatStringPos);
		m_FormatStringPos=string_type::npos;
	}
}

template<class Ch, class Tr>
std::basic_ostream<Ch, Tr>&
operator<<(std::basic_ostream<Ch, Tr>& os, basic_string_format<Ch, Tr>& sf)
{
	os<<sf.str();
	return os;
}

typedef basic_string_format<char> string_format;
typedef basic_string_format<wchar_t> wstring_format;

#ifdef _UNICODE
typedef wstring_format tstring_format;
#else
typedef string_format tstring_format;
#endif

#endif // !defined(AFX_STRING_FORMAT_H__8A4AE1C8_3D8A_40C3_B56E_0E619BC3BFA0__INCLUDED_)
