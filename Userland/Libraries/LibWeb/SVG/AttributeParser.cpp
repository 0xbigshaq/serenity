/*
 * Copyright (c) 2020, Matthew Olsson <mattco@serenityos.org>
 * Copyright (c) 2022, Sam Atkins <atkinssj@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AttributeParser.h"
#include <AK/StringBuilder.h>
#include <ctype.h>

namespace Web::SVG {

AttributeParser::AttributeParser(String source)
    : m_source(move(source))
{
}

Vector<PathInstruction> AttributeParser::parse_path_data()
{
    parse_whitespace();
    while (!done())
        parse_drawto();
    if (!m_instructions.is_empty() && m_instructions[0].type != PathInstructionType::Move)
        VERIFY_NOT_REACHED();
    return m_instructions;
}

void AttributeParser::parse_drawto()
{
    if (match('M') || match('m')) {
        parse_moveto();
    } else if (match('Z') || match('z')) {
        parse_closepath();
    } else if (match('L') || match('l')) {
        parse_lineto();
    } else if (match('H') || match('h')) {
        parse_horizontal_lineto();
    } else if (match('V') || match('v')) {
        parse_vertical_lineto();
    } else if (match('C') || match('c')) {
        parse_curveto();
    } else if (match('S') || match('s')) {
        parse_smooth_curveto();
    } else if (match('Q') || match('q')) {
        parse_quadratic_bezier_curveto();
    } else if (match('T') || match('t')) {
        parse_smooth_quadratic_bezier_curveto();
    } else if (match('A') || match('a')) {
        parse_elliptical_arc();
    } else {
        dbgln("AttributeParser::parse_drawto failed to match: '{}'", ch());
        TODO();
    }
}

void AttributeParser::parse_moveto()
{
    bool absolute = consume() == 'M';
    parse_whitespace();
    for (auto& pair : parse_coordinate_pair_sequence())
        m_instructions.append({ PathInstructionType::Move, absolute, pair });
}

void AttributeParser::parse_closepath()
{
    bool absolute = consume() == 'Z';
    parse_whitespace();
    m_instructions.append({ PathInstructionType::ClosePath, absolute, {} });
}

void AttributeParser::parse_lineto()
{
    bool absolute = consume() == 'L';
    parse_whitespace();
    for (auto& pair : parse_coordinate_pair_sequence())
        m_instructions.append({ PathInstructionType::Line, absolute, pair });
}

void AttributeParser::parse_horizontal_lineto()
{
    bool absolute = consume() == 'H';
    parse_whitespace();
    m_instructions.append({ PathInstructionType::HorizontalLine, absolute, parse_coordinate_sequence() });
}

void AttributeParser::parse_vertical_lineto()
{
    bool absolute = consume() == 'V';
    parse_whitespace();
    m_instructions.append({ PathInstructionType::VerticalLine, absolute, parse_coordinate_sequence() });
}

void AttributeParser::parse_curveto()
{
    bool absolute = consume() == 'C';
    parse_whitespace();

    while (true) {
        m_instructions.append({ PathInstructionType::Curve, absolute, parse_coordinate_pair_triplet() });
        if (match_comma_whitespace())
            parse_comma_whitespace();
        if (!match_coordinate())
            break;
    }
}

void AttributeParser::parse_smooth_curveto()
{
    bool absolute = consume() == 'S';
    parse_whitespace();

    while (true) {
        m_instructions.append({ PathInstructionType::SmoothCurve, absolute, parse_coordinate_pair_double() });
        if (match_comma_whitespace())
            parse_comma_whitespace();
        if (!match_coordinate())
            break;
    }
}

void AttributeParser::parse_quadratic_bezier_curveto()
{
    bool absolute = consume() == 'Q';
    parse_whitespace();

    while (true) {
        m_instructions.append({ PathInstructionType::QuadraticBezierCurve, absolute, parse_coordinate_pair_double() });
        if (match_comma_whitespace())
            parse_comma_whitespace();
        if (!match_coordinate())
            break;
    }
}

void AttributeParser::parse_smooth_quadratic_bezier_curveto()
{
    bool absolute = consume() == 'T';
    parse_whitespace();

    while (true) {
        m_instructions.append({ PathInstructionType::SmoothQuadraticBezierCurve, absolute, parse_coordinate_pair() });
        if (match_comma_whitespace())
            parse_comma_whitespace();
        if (!match_coordinate())
            break;
    }
}

void AttributeParser::parse_elliptical_arc()
{
    bool absolute = consume() == 'A';
    parse_whitespace();

    while (true) {
        m_instructions.append({ PathInstructionType::EllipticalArc, absolute, parse_elliptical_arg_argument() });
        if (match_comma_whitespace())
            parse_comma_whitespace();
        if (!match_coordinate())
            break;
    }
}

float AttributeParser::parse_coordinate()
{
    return parse_sign() * parse_number();
}

Vector<float> AttributeParser::parse_coordinate_pair()
{
    Vector<float> coordinates;
    coordinates.append(parse_coordinate());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    coordinates.append(parse_coordinate());
    return coordinates;
}

Vector<float> AttributeParser::parse_coordinate_sequence()
{
    Vector<float> sequence;
    while (true) {
        sequence.append(parse_coordinate());
        if (match_comma_whitespace())
            parse_comma_whitespace();
        if (!match_comma_whitespace() && !match_coordinate())
            break;
    }
    return sequence;
}

Vector<Vector<float>> AttributeParser::parse_coordinate_pair_sequence()
{
    Vector<Vector<float>> sequence;
    while (true) {
        sequence.append(parse_coordinate_pair());
        if (match_comma_whitespace())
            parse_comma_whitespace();
        if (!match_comma_whitespace() && !match_coordinate())
            break;
    }
    return sequence;
}

Vector<float> AttributeParser::parse_coordinate_pair_double()
{
    Vector<float> coordinates;
    coordinates.extend(parse_coordinate_pair());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    coordinates.extend(parse_coordinate_pair());
    return coordinates;
}

Vector<float> AttributeParser::parse_coordinate_pair_triplet()
{
    Vector<float> coordinates;
    coordinates.extend(parse_coordinate_pair());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    coordinates.extend(parse_coordinate_pair());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    coordinates.extend(parse_coordinate_pair());
    return coordinates;
}

Vector<float> AttributeParser::parse_elliptical_arg_argument()
{
    Vector<float> numbers;
    numbers.append(parse_number());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    numbers.append(parse_number());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    numbers.append(parse_number());
    parse_comma_whitespace();
    numbers.append(parse_flag());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    numbers.append(parse_flag());
    if (match_comma_whitespace())
        parse_comma_whitespace();
    numbers.extend(parse_coordinate_pair());

    return numbers;
}

void AttributeParser::parse_whitespace(bool must_match_once)
{
    bool matched = false;
    while (!done() && match_whitespace()) {
        consume();
        matched = true;
    }

    VERIFY(!must_match_once || matched);
}

void AttributeParser::parse_comma_whitespace()
{
    if (match(',')) {
        consume();
        parse_whitespace();
    } else {
        parse_whitespace(1);
        if (match(','))
            consume();
        parse_whitespace();
    }
}

float AttributeParser::parse_fractional_constant()
{
    StringBuilder builder;
    bool floating_point = false;

    while (!done() && isdigit(ch()))
        builder.append(consume());

    if (match('.')) {
        floating_point = true;
        builder.append('.');
        consume();
        while (!done() && isdigit(ch()))
            builder.append(consume());
    } else {
        VERIFY(builder.length() > 0);
    }

    if (floating_point)
        return strtof(builder.to_string().characters(), nullptr);
    return builder.to_string().to_int().value();
}

float AttributeParser::parse_number()
{
    auto number = parse_fractional_constant();

    if (!match('e') && !match('E'))
        return number;
    consume();

    auto exponent_sign = parse_sign();

    StringBuilder exponent_builder;
    while (!done() && isdigit(ch()))
        exponent_builder.append(consume());
    VERIFY(exponent_builder.length() > 0);

    auto exponent = exponent_builder.to_string().to_int().value();

    // Fast path: If the number is 0, there's no point in computing the exponentiation.
    if (number == 0)
        return number;

    if (exponent_sign < 0) {
        for (int i = 0; i < exponent; ++i) {
            number /= 10;
        }
    } else if (exponent_sign > 0) {
        for (int i = 0; i < exponent; ++i) {
            number *= 10;
        }
    }

    return number;
}

float AttributeParser::parse_flag()
{
    if (!match('0') && !match('1'))
        VERIFY_NOT_REACHED();
    return consume() - '0';
}

int AttributeParser::parse_sign()
{
    if (match('-')) {
        consume();
        return -1;
    }
    if (match('+'))
        consume();
    return 1;
}

bool AttributeParser::match_whitespace() const
{
    if (done())
        return false;
    char c = ch();
    return c == 0x9 || c == 0x20 || c == 0xa || c == 0xc || c == 0xd;
}

bool AttributeParser::match_comma_whitespace() const
{
    return match_whitespace() || match(',');
}

bool AttributeParser::match_coordinate() const
{
    return !done() && (isdigit(ch()) || ch() == '-' || ch() == '+' || ch() == '.');
}

}