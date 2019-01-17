/* Copyright 2017-2018 Sander van Geloven
 *
 * This file is part of Nuspell.
 *
 * Nuspell is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nuspell is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nuspell.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <nuspell/locale_utils.hxx>

#include <algorithm>

#include <boost/locale/utf8_codecvt.hpp>
#include <catch2/catch.hpp>

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

TEST_CASE("method validate_utf8", "[locale_utils]")
{
	CHECK(validate_utf8(""s));
	CHECK(validate_utf8("the brown fox~"s));
	CHECK(validate_utf8("Ӥ日本に"s));
	// need counter example too
}

TEST_CASE("method is_ascii", "[locale_utils]")
{
	CHECK(is_ascii('a'));
	CHECK(is_ascii('\t'));

	CHECK_FALSE(is_ascii(char_traits<char>::to_char_type(128)));
}

TEST_CASE("method is_all_ascii", "[locale_utils]")
{
	CHECK(is_all_ascii(""s));
	CHECK(is_all_ascii("the brown fox~"s));
	CHECK_FALSE(is_all_ascii("brown foxĳӤ"s));
}

TEST_CASE("method latin1_to_ucs2", "[locale_utils]")
{
	CHECK(u"" == latin1_to_ucs2(""s));
	CHECK(u"abc\u0080" == latin1_to_ucs2("abc\x80"s));
	CHECK(u"²¿ýþÿ" != latin1_to_ucs2(u8"²¿ýþÿ"s));
	CHECK(u"Ӥ日本に" != latin1_to_ucs2(u8"Ӥ日本に"s));
}

TEST_CASE("method is_all_bmp", "[locale_utils]")
{
	CHECK(true == is_all_bmp(u"abcýþÿӤ"));
	CHECK(false == is_all_bmp(u"abcý \U00010001 þÿӤ"));
}

using namespace boost::locale;

template <typename CharType>
class latin1_codecvt
    : public generic_codecvt<CharType, latin1_codecvt<CharType>> {
      public:
	/* Standard codecvt constructor */
	latin1_codecvt(size_t refs = 0)
	    : generic_codecvt<CharType, latin1_codecvt<CharType>>(refs)
	{
	}
	/* State is unused but required by generic_codecvt */
	struct state_type {
	};
	state_type initial_state(
	    generic_codecvt_base::initial_convertion_state /*unused*/) const
	{
		return state_type();
	}

	int max_encoding_length() const { return 1; }
	utf::code_point to_unicode(state_type&, char const*& begin,
	                           char const* end) const
	{
		if (begin == end)
			return utf::incomplete;
		return static_cast<unsigned char>(*begin++);
	}
	utf::code_point from_unicode(state_type&, utf::code_point u,
	                             char* begin, char const* end) const
	{
		if (u >= 256)
			return utf::illegal;
		if (begin == end)
			return utf::incomplete;
		*begin = u;
		return 1;
	}
};

TEST_CASE("to_wide", "[locale_utils]")
{
	auto loc = locale(locale::classic(), new utf8_codecvt<wchar_t>());
	auto in = u8"\U0010FFFF ß"s;
	CHECK(L"\U0010FFFF ß" == to_wide(in, loc));

	in = u8"\U00011D59\U00011D59\U00011D59\U00011D59\U00011D59"s;
	auto out = wstring();
	auto exp = L"\U00011D59\U00011D59\U00011D59\U00011D59\U00011D59";
	CHECK(true == to_wide(in, loc, out));
	CHECK(exp == out);

	loc = locale(locale::classic(), new latin1_codecvt<wchar_t>());
	in = "abcd\xDF";
	CHECK(L"abcdß" == to_wide(in, loc));
}

TEST_CASE("to_narrow", "[locale_utils]")
{
	auto loc = locale(locale::classic(), new utf8_codecvt<wchar_t>());
	auto in = L"\U0010FFFF ß"s;
	CHECK(u8"\U0010FFFF ß" == to_narrow(in, loc));

	in = L"\U00011D59\U00011D59\U00011D59\U00011D59\U00011D59";
	auto out = string();
	CHECK(true == to_narrow(in, out, loc));
	CHECK("\U00011D59\U00011D59\U00011D59\U00011D59\U00011D59" == out);

	loc = locale(locale::classic(), new latin1_codecvt<wchar_t>());
	in = L"abcdß";
	CHECK("abcd\xDF" == to_narrow(in, loc));

	in = L"\U00011D59\U00011D59\U00011D59\U00011D59\U00011D59";
	out = string();
	CHECK(false == to_narrow(in, out, loc));
	CHECK(all_of(begin(out), end(out), [](auto c) { return c == '?'; }));
}

TEST_CASE("method classify_casing", "[locale_utils]")
{
	CHECK(Casing::SMALL == classify_casing(L""s));
	CHECK(Casing::SMALL == classify_casing(L"alllowercase"s));
	CHECK(Casing::SMALL == classify_casing(L"alllowercase3"s));
	CHECK(Casing::INIT_CAPITAL == classify_casing(L"Initandlowercase"s));
	CHECK(Casing::INIT_CAPITAL == classify_casing(L"Initandlowercase_"s));
	CHECK(Casing::ALL_CAPITAL == classify_casing(L"ALLUPPERCASE"s));
	CHECK(Casing::ALL_CAPITAL == classify_casing(L"ALLUPPERCASE."s));
	CHECK(Casing::CAMEL == classify_casing(L"iCamelCase"s));
	CHECK(Casing::CAMEL == classify_casing(L"iCamelCase@"s));
	CHECK(Casing::PASCAL == classify_casing(L"InitCamelCase"s));
	CHECK(Casing::PASCAL == classify_casing(L"InitCamelCase "s));
	CHECK(Casing::INIT_CAPITAL == classify_casing(L"İstanbul"s));
}

TEST_CASE("to_upper", "[locale_utils]")
{
	auto l = icu::Locale();

	CHECK(L"" == to_upper(L"", l));
	CHECK(L"A" == to_upper(L"a", l));
	CHECK(L"A" == to_upper(L"A", l));
	CHECK(L"AA" == to_upper(L"aa", l));
	CHECK(L"AA" == to_upper(L"aA", l));
	CHECK(L"AA" == to_upper(L"Aa", l));
	CHECK(L"AA" == to_upper(L"AA", l));

	CHECK(L"TABLE" == to_upper(L"table", l));
	CHECK(L"TABLE" == to_upper(L"Table", l));
	CHECK(L"TABLE" == to_upper(L"tABLE", l));
	CHECK(L"TABLE" == to_upper(L"TABLE", l));

	// Note that i is converted to I, not İ
	CHECK_FALSE(L"İSTANBUL" == to_upper(L"istanbul", l));

	l = icu::Locale("tr_TR");
	CHECK(L"İSTANBUL" == to_upper(L"istanbul", l));
	// Note that I remains and is not converted to İ
	CHECK_FALSE(L"İSTANBUL" == to_upper(L"Istanbul", l));
	CHECK(L"DİYARBAKIR" == to_upper(L"Diyarbakır", l));

	l = icu::Locale("de_DE");
	// Note that lower case ü is not converted to upper case Ü.
	// Note that lower case ß is converted to double SS.
	// CHECK(L"GRüSSEN" == to_upper(L"grüßen", l));
	CHECK(L"GRÜSSEN" == to_upper(L"GRÜßEN", l));
	// Note that upper case ẞ is kept in upper case.
	CHECK(L"GRÜẞEN" == to_upper(L"GRÜẞEN", l));

	l = icu::Locale("nl_NL");
	CHECK(L"ÉÉN" == to_upper(L"één", l));
	CHECK(L"ÉÉN" == to_upper(L"Één", l));
	CHECK(L"IJSSELMEER" == to_upper(L"ijsselmeer", l));
	CHECK(L"IJSSELMEER" == to_upper(L"IJsselmeer", l));
	CHECK(L"IJSSELMEER" == to_upper(L"IJSSELMEER", l));
	CHECK(L"ĲSSELMEER" == to_upper(L"ĳsselmeer", l));
	CHECK(L"ĲSSELMEER" == to_upper(L"Ĳsselmeer", l));
	CHECK(L"ĲSSELMEER" == to_upper(L"ĲSSELMEER", l));
}

TEST_CASE("to_lower", "[locale_utils]")
{
	auto l = icu::Locale("en_US");

	CHECK(L"" == to_lower(L"", l));
	CHECK(L"a" == to_lower(L"A", l));
	CHECK(L"a" == to_lower(L"a", l));
	CHECK(L"aa" == to_lower(L"aa", l));
	CHECK(L"aa" == to_lower(L"aA", l));
	CHECK(L"aa" == to_lower(L"Aa", l));
	CHECK(L"aa" == to_lower(L"AA", l));

	CHECK(L"table" == to_lower(L"table", l));
	CHECK(L"table" == to_lower(L"Table", l));
	CHECK(L"table" == to_lower(L"TABLE", l));

	// Note that İ is converted to i followed by COMBINING DOT ABOVE U+0307
	CHECK_FALSE(L"istanbul" == to_lower(L"İSTANBUL", l));
	// Note that İ is converted to i followed by COMBINING DOT ABOVE U+0307
	CHECK_FALSE(L"istanbul" == to_lower(L"İstanbul", l));

	l = icu::Locale("tr_TR");
	CHECK(L"istanbul" == to_lower(L"İSTANBUL", l));
	CHECK(L"istanbul" == to_lower(L"İstanbul", l));
	CHECK(L"diyarbakır" == to_lower(L"Diyarbakır", l));

	l = icu::Locale("el_GR");
	CHECK(L"ελλάδα" == to_lower(L"ελλάδα", l));
	CHECK(L"ελλάδα" == to_lower(L"Ελλάδα", l));
	CHECK(L"ελλάδα" == to_lower(L"ΕΛΛΆΔΑ", l));

	l = icu::Locale("de_DE");
	CHECK(L"grüßen" == to_lower(L"grüßen", l));
	CHECK(L"grüssen" == to_lower(L"grüssen", l));
	// Note that double SS is not converted to lower case ß.
	CHECK(L"grüssen" == to_lower(L"GRÜSSEN", l));
	// Note that upper case ẞ is converted to lower case ß.
	// this assert fails on windows with icu 62
	// CHECK(L"grüßen" == to_lower(L"GRÜẞEN", l));

	l = icu::Locale("nl_NL");
	CHECK(L"één" == to_lower(L"Één", l));
	CHECK(L"één" == to_lower(L"ÉÉN", l));
	CHECK(L"ijsselmeer" == to_lower(L"ijsselmeer", l));
	CHECK(L"ijsselmeer" == to_lower(L"IJsselmeer", l));
	CHECK(L"ijsselmeer" == to_lower(L"IJSSELMEER", l));
	CHECK(L"ĳsselmeer" == to_lower(L"Ĳsselmeer", l));
	CHECK(L"ĳsselmeer" == to_lower(L"ĲSSELMEER", l));
	CHECK(L"ĳsselmeer" == to_lower(L"Ĳsselmeer", l));
}

TEST_CASE("to_title", "[locale_utils]")
{
	auto l = icu::Locale("en_US");
	CHECK(L"" == to_title(L""s, l));
	CHECK(L"A" == to_title(L"a", l));
	CHECK(L"A" == to_title(L"A", l));
	CHECK(L"Aa" == to_title(L"aa", l));
	CHECK(L"Aa" == to_title(L"Aa", l));
	CHECK(L"Aa" == to_title(L"aA", l));
	CHECK(L"Aa" == to_title(L"AA", l));

	CHECK(L"Table" == to_title(L"table", l));
	CHECK(L"Table" == to_title(L"Table", l));
	CHECK(L"Table" == to_title(L"tABLE", l));
	CHECK(L"Table" == to_title(L"TABLE", l));

	// Note that i is converted to I, not İ
	CHECK_FALSE(L"İstanbul" == to_title(L"istanbul", l));
	// Note that i is converted to I, not İ
	CHECK_FALSE(L"İstanbul" == to_title(L"iSTANBUL", l));
	CHECK(L"İstanbul" == to_title(L"İSTANBUL", l));
	CHECK(L"Istanbul" == to_title(L"ISTANBUL", l));

	l = icu::Locale("tr_TR");
	CHECK(L"İstanbul" == to_title(L"istanbul", l));
	CHECK(L"İstanbul" == to_title(L"iSTANBUL", l));
	CHECK(L"İstanbul" == to_title(L"İSTANBUL", l));
	CHECK(L"Istanbul" == to_title(L"ISTANBUL", l));
	CHECK(L"Diyarbakır" == to_title(L"diyarbakır", l));
	l = icu::Locale("tr_CY");
	CHECK(L"İstanbul" == to_title(L"istanbul", l));
	l = icu::Locale("crh_UA");
	// Note that lower case i is not converted to upper case İ, bug?
	CHECK(L"Istanbul" == to_title(L"istanbul", l));
	l = icu::Locale("az_AZ");
	CHECK(L"İstanbul" == to_title(L"istanbul", l));
	l = icu::Locale("az_IR");
	CHECK(L"İstanbul" == to_title(L"istanbul", l));

	l = icu::Locale("el_GR");
	CHECK(L"Ελλάδα" == to_title(L"ελλάδα", l));
	CHECK(L"Ελλάδα" == to_title(L"Ελλάδα", l));
	CHECK(L"Ελλάδα" == to_title(L"ΕΛΛΆΔΑ", l));
	CHECK(L"Σίγμα" == to_title(L"Σίγμα", l));
	CHECK(L"Σίγμα" == to_title(L"σίγμα", l));
	// Use of ς where σ is expected, should convert to upper case Σ.
	CHECK(L"Σίγμα" == to_title(L"ςίγμα", l));

	l = icu::Locale("de_DE");
	CHECK(L"Grüßen" == to_title(L"grüßen", l));
	CHECK(L"Grüßen" == to_title(L"GRÜßEN", l));
	// Use of upper case ẞ where lower case ß is expected.
	// this assert fails on windows with icu 62
	// CHECK(L"Grüßen" == to_title(L"GRÜẞEN", l));

	l = icu::Locale("nl_NL");
	CHECK(L"Één" == to_title(L"één", l));
	CHECK(L"Één" == to_title(L"ÉÉN", l));
	CHECK(L"IJsselmeer" == to_title(L"ijsselmeer", l));
	CHECK(L"IJsselmeer" == to_title(L"Ijsselmeer", l));
	CHECK(L"IJsselmeer" == to_title(L"iJsselmeer", l));
	CHECK(L"IJsselmeer" == to_title(L"IJsselmeer", l));
	CHECK(L"IJsselmeer" == to_title(L"IJSSELMEER", l));
	CHECK(L"Ĳsselmeer" == to_title(L"ĳsselmeer", l));
	CHECK(L"Ĳsselmeer" == to_title(L"Ĳsselmeer", l));
	CHECK(L"Ĳsselmeer" == to_title(L"ĲSSELMEER", l));
}

TEST_CASE("Encoding", "[locale_utils]")
{
	auto e = Encoding();
	auto v = e.value_or_default();
	auto i = e.is_utf8();
	CHECK("ISO8859-1" == v);
	CHECK(false == i);

	e = Encoding("UTF8");
	v = e.value();
	i = e.is_utf8();
	CHECK("UTF-8" == v);
	CHECK(true == i);

	e = "MICROSOFT-CP1251";
	v = e.value();
	i = e.is_utf8();
	CHECK("CP1251" == v);
	CHECK(false == i);
}
