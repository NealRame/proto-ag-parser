/*
 * main.cpp
 *
 *  Created on: Jul 12, 2013
 *      Author: jux
 */

#include <limits>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <boost/format.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/boost_tuple.hpp>

std::string readAll(std::istream &in) {
	std::ostringstream out;
	char buf[64];
	do {
		std::streamsize n;
		in.read(buf, sizeof(buf));
		if ((n = in.gcount()) > 0) {
			out.write(buf, n);
		}
	} while (in.good());
	return out.str();
}

namespace parser {
#	define BYTE_TO_DOUBLE(VALUE) \
		static_cast<double>(VALUE)/std::numeric_limits<unsigned char>::max()

	using namespace boost::spirit;

	typedef boost::tuple<double, double, double, double> RGBA_Attr;	

	struct hex1_ : qi::symbols<char, double> {
		hex1_() {
			add
				("0", 0)
				("1", BYTE_TO_DOUBLE(0x11))
				("2", BYTE_TO_DOUBLE(0x22))
				("3", BYTE_TO_DOUBLE(0x33))
				("4", BYTE_TO_DOUBLE(0x44))
				("5", BYTE_TO_DOUBLE(0x55))
				("6", BYTE_TO_DOUBLE(0x66))
				("7", BYTE_TO_DOUBLE(0x77))
				("8", BYTE_TO_DOUBLE(0x88))
				("9", BYTE_TO_DOUBLE(0x99))
				("a", BYTE_TO_DOUBLE(0xaa))
				("b", BYTE_TO_DOUBLE(0xbb))
				("c", BYTE_TO_DOUBLE(0xcc))
				("d", BYTE_TO_DOUBLE(0xdd))
				("e", BYTE_TO_DOUBLE(0xee))
				("f", BYTE_TO_DOUBLE(0xff))
			;
		}
	} hex1;

	struct hex2_ : qi::symbols<char, double> {
		hex2_() {
			for (unsigned int i=0; i<=0xff; ++i) {
				add((boost::format("%02x") % i).str(),
					BYTE_TO_DOUBLE(i));
			}
		}
	} hex2;

	template <typename Iterator> 
	struct ParseError {
		typedef Iterator iterator;

		ParseError(Iterator f, Iterator l, Iterator p, qi::info w) :
			first(f), last(l), error_pos(p), what(w) { 
		}

		Iterator first;
		Iterator last;
		Iterator error_pos;
		qi::info what;
	};


	template <typename Iterator>
	struct ColorGrammar : 
		qi::grammar<Iterator, RGBA_Attr(), ascii::space_type>
	{
		static void RGB_check_component(
				double const &v, qi::unused_type, bool &ok) {
			ok = (v >= 0 && v <= 1);
		}

		ColorGrammar() : ColorGrammar::base_type(color, "color declaration") {
			using qi::_val;
			using qi::lit;
			using qi::lexeme;
			using qi::no_case;
			using namespace qi::labels;
			using boost::phoenix::at_c;
			using boost::phoenix::construct;
			using boost::phoenix::val;

			color.name("color declaration");
			color %= hex_short_expr | hex_long_expr | rgb_expr | rgba_expr;

			hex_long_expr.name("hexadecimal color constant");
			hex_long_expr %=
				no_case[
					lexeme[ lit('#') 
						>> hex2 >> hex2 >> hex2 >> default_alpha
					]
				];

			hex_short_expr.name("hexadecimal color constant");
			hex_short_expr %=
				no_case[
					lexeme[ lit('#')
						>> hex1 >> hex1 >> hex1 >> default_alpha
					]
				];

			rgb_expr.name("RGB color expression");
			rgb_expr %=
				no_case[
					lit("rgb") >> lit('(')
					>> double_ [RGB_check_component]
					>> lit(',')
					>> double_ [RGB_check_component]
					>> lit(',')
					>> double_ [RGB_check_component]
					>> default_alpha // see default_alpha definition
					>> lit(')')
				];

			rgba_expr.name("RGB color expression");
			rgba_expr =
				no_case[
					lit("rgb") >> lit('(')
					>> double_ [RGB_check_component]
					>> lit(',')
					>> double_ [RGB_check_component]
					>> lit(',')
					>> double_ [RGB_check_component]
					>> lit(',')
					>> double_ [RGB_check_component]
					>> lit(')')
				];

			default_alpha = eps [_val = 1];
		}
		
		qi::rule<Iterator, double()> default_alpha;
		qi::rule<Iterator, RGBA_Attr(), ascii::space_type> color;
		qi::rule<Iterator, RGBA_Attr(), ascii::space_type> hex_long_expr;
		qi::rule<Iterator, RGBA_Attr(), ascii::space_type> hex_short_expr;
		qi::rule<Iterator, RGBA_Attr(), ascii::space_type> rgb_expr;
		qi::rule<Iterator, RGBA_Attr(), ascii::space_type> rgba_expr;
	};

	template <typename Iterator>
	bool parse_color(Iterator first, Iterator last, RGBA_Attr &value) {
		ColorGrammar<Iterator> grammar;
		return qi::phrase_parse(first, last, grammar, ascii::space, value);
	}

}  // namespace parser

#define PRINT_RGBA_ATTR(RGBA) do \
{ \
	std::cout \
		<<   "r=" << (RGBA).get<0>() \
		<< ", g=" << (RGBA).get<1>() \
		<< ", b=" << (RGBA).get<2>() \
		<< ", a=" << (RGBA).get<3>() \
		<< std::endl; \
} while (0)

int main(int argc, char **argv) {

	{
		std::ifstream in("./tests/test1.txt");
		std::string s(readAll(in));
		parser::RGBA_Attr value;
		if (! parser::parse_color(s.begin(), s.end(), value)) {
			std::cerr << "parse error" << std::endl;
		} else {
			PRINT_RGBA_ATTR(value);
		}
	}

	{
		std::ifstream in("./tests/test2.txt");
		std::string s(readAll(in));
		parser::RGBA_Attr value;
		if (! parser::parse_color(s.begin(), s.end(), value)) {
			std::cerr << "parse error" << std::endl;
		} else {
			PRINT_RGBA_ATTR(value);
		}
	}

	{
		std::ifstream in("./tests/test3.txt");
		std::string s(readAll(in));
		parser::RGBA_Attr value;
		if (! parser::parse_color(s.begin(), s.end(), value)) {
			std::cerr << "parse error" << std::endl;
		} else {
			PRINT_RGBA_ATTR(value);
		}
	}

	{
		std::ifstream in("./tests/test4.txt");
		std::string s(readAll(in));
		parser::RGBA_Attr value;
		if (! parser::parse_color(s.begin(), s.end(), value)) {
			std::cerr << "parse error" << std::endl;
		} else {
			PRINT_RGBA_ATTR(value);
		}
	}

	return 0;
}
