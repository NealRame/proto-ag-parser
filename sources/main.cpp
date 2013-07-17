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
		static_cast<double>((unsigned char)(VALUE))/std::numeric_limits<unsigned char>::max()

	using namespace boost::spirit;

	typedef boost::tuple<double, double, double, double> RGBA_Attr;
	typedef boost::tuple<RGBA_Attr, double> GradientStop_Attr;
	typedef std::vector<GradientStop_Attr> Gradient_Attr;

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
	struct ColorGrammar : 
		qi::grammar<Iterator, RGBA_Attr(), ascii::space_type>
	{
		static bool parse(Iterator &first, Iterator last, RGBA_Attr &value) {
			ColorGrammar<Iterator> grammar;
			if (! qi::phrase_parse(first, last, grammar, ascii::space, value)) {
				// find a solution to correctly report an error
				return false;
			}
			return true;
		}

		ColorGrammar() : 
			ColorGrammar::base_type(color, "color") {
			using qi::_val;
			using qi::attr;
			using qi::lexeme;
			using qi::lit;
			using qi::no_case;
			using boost::phoenix::at_c;
			using boost::phoenix::construct;
			using boost::phoenix::val;
			using boost::phoenix::ref;

			color.name("color");
			color %= hex_expr | rgb_expr | rgba_expr;


			hex_expr.name("hexadecimal color constant");
			hex_expr %= // hex_long_expr must be check before hex_short_expr
				hex_long_expr | hex_short_expr;

			hex_long_expr.name("hexadecimal color constant (long form)");
			hex_long_expr %=
				no_case[
					lexeme[ lit('#')
						>> (hex2)
						>> (hex2)
						>> (hex2)
						>> attr(1.0)
					]
				];

			hex_short_expr.name("hexadecimal color constant (short form)");
			hex_short_expr %=
				no_case[
					lexeme[ lit('#')
						>> hex1
						>> hex1
						>> hex1
						>> attr(1.0)
					]
				];

			rgb_expr.name("RGB color");
			rgb_expr %=
				no_case[
					lit("rgb") >> lit('(')
					>> double_ [check_RGBA_component]
					>> lit(',')
					>> double_ [check_RGBA_component]
					>> lit(',')
					>> double_ [check_RGBA_component]
					>> lit(')')
					>> attr(1.0)
				];

			rgba_expr.name("RGBA color");
			rgba_expr %=
				no_case[
					lit("rgba") >> lit('(')
					>> double_ [check_RGBA_component]
					>> lit(',')
					>> double_ [check_RGBA_component]
					>> lit(',')
					>> double_ [check_RGBA_component]
					>> lit(',')
					>> double_ [check_RGBA_component]
					>> lit(')')
				];
		}
		
		qi::rule<Iterator, RGBA_Attr(), ascii::space_type> color;
		qi::rule<Iterator, RGBA_Attr(), ascii::space_type> hex_expr;
		qi::rule<Iterator, RGBA_Attr(), ascii::space_type> hex_long_expr;
		qi::rule<Iterator, RGBA_Attr(), ascii::space_type> hex_short_expr;
		qi::rule<Iterator, RGBA_Attr(), ascii::space_type> rgb_expr;
		qi::rule<Iterator, RGBA_Attr(), ascii::space_type> rgba_expr;

	private:
		static void check_RGBA_component(
			double const &v, qi::unused_type, bool &ok) {
			ok = (v >= 0 && v <= 1);
		}
	};

	template <typename Iterator>
	struct GradientGrammar :
		qi::grammar<Iterator, Gradient_Attr(), ascii::space_type>
	{
		static bool parse(Iterator &first, Iterator last, Gradient_Attr &value) {
			GradientGrammar<Iterator> grammar;
			if (! qi::phrase_parse(first, last, grammar, ascii::space, value)) {
				// find a solution to correctly report an error
				return false;
			}
			return true;
		}

		GradientGrammar() :
			GradientGrammar::base_type(gradient, "gradient") {
			using qi::lexeme;
			using qi::lit;
			using qi::no_case;

			gradient %=
				no_case[
					lit("gradient") 
					>> lit('(') 
					>> gradient_stop % lit(',') 
					>> lit(')')
				];

			gradient_stop %=
				color_grammar.color >> double_[check_stop_offset];

		}

		ColorGrammar<Iterator> color_grammar;
		qi::rule<Iterator, Gradient_Attr(), ascii::space_type> gradient;
		qi::rule<Iterator, GradientStop_Attr(), ascii::space_type> gradient_stop;

	private:
		static void check_stop_offset(
			double const &v, qi::unused_type, bool &ok) {
			ok = (v >= 0 && v <= 1);
		}
	};

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

#define PRINT_GRADIENT_ATTR(GRADIENT) \
do { \
	std::cout << "---" << std::endl; \
	for (const parser::GradientStop_Attr &gradient_stop : (GRADIENT)) { \
		std::cout << gradient_stop.get<1>() << ": "; \
		parser::RGBA_Attr color = gradient_stop.get<0>(); \
		PRINT_RGBA_ATTR(color); \
	} \
	std::cout << "---" << std::endl; \
} while (false)

int main(int argc, char **argv) {
	// COLOR PARSE TEST
	{
		std::cout << "COLOR PARSE TEST" << std::endl;

		std::ifstream in("./tests/test1.txt");
		std::string s(readAll(in));
		std::string::iterator it = s.begin(), end = s.end();

		while (it != end) {
			parser::RGBA_Attr value;
			if (! parser::ColorGrammar<std::string::iterator>::parse(it, s.end(), value)) {
				std::cerr << "parse error" << std::endl;
				while (it != end && *it != '\n') {
					++it;
				}
			} else {
				PRINT_RGBA_ATTR(value);
			}
		}
	}

	// GRADIENT PARSE TEST
	{
		std::cout << "GRADIENT PARSE TEST" << std::endl;
		std::ifstream in("./tests/test2.txt");
		std::string s(readAll(in));
		std::string::iterator it = s.begin(), end = s.end();

		while (it != end) {
			parser::Gradient_Attr value;
			if (! parser::GradientGrammar<std::string::iterator>::parse(it, s.end(), value)) {
				std::cerr << "parse error" << std::endl;
				while (it != end && *it != '\n') {
					++it;
				}
			} else {
				PRINT_GRADIENT_ATTR(value);
			}
		}
	}

	return 0;
}
