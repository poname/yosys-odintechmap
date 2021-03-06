/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Claire Xenia Wolf <claire@yosyshq.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "kernel/celledges.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

void bitwise_unary_op(AbstractCellEdgesDatabase *db, RTLIL::Cell *cell)
{
	bool is_signed = cell->getParam(ID::A_SIGNED).as_bool();
	int a_width = GetSize(cell->getPort(ID::A));
	int y_width = GetSize(cell->getPort(ID::Y));

	for (int i = 0; i < y_width; i++)
	{
		if (i < a_width)
			db->add_edge(cell, ID::A, i, ID::Y, i, -1);
		else if (is_signed && a_width > 0)
			db->add_edge(cell, ID::A, a_width-1, ID::Y, i, -1);
	}
}

void bitwise_binary_op(AbstractCellEdgesDatabase *db, RTLIL::Cell *cell)
{
	bool is_signed = cell->getParam(ID::A_SIGNED).as_bool();
	int a_width = GetSize(cell->getPort(ID::A));
	int b_width = GetSize(cell->getPort(ID::B));
	int y_width = GetSize(cell->getPort(ID::Y));

	if (cell->type == ID($and) && !is_signed) {
		if (a_width > b_width)
			a_width = b_width;
		else
			b_width = a_width;
	}

	for (int i = 0; i < y_width; i++)
	{
		if (i < a_width)
			db->add_edge(cell, ID::A, i, ID::Y, i, -1);
		else if (is_signed && a_width > 0)
			db->add_edge(cell, ID::A, a_width-1, ID::Y, i, -1);

		if (i < b_width)
			db->add_edge(cell, ID::B, i, ID::Y, i, -1);
		else if (is_signed && b_width > 0)
			db->add_edge(cell, ID::B, b_width-1, ID::Y, i, -1);
	}
}

void arith_neg_op(AbstractCellEdgesDatabase *db, RTLIL::Cell *cell)
{
	bool is_signed = cell->getParam(ID::A_SIGNED).as_bool();
	int a_width = GetSize(cell->getPort(ID::A));
	int y_width = GetSize(cell->getPort(ID::Y));

	if (is_signed && a_width == 1)
		y_width = std::min(y_width, 1);

	for (int i = 0; i < y_width; i++)
	for (int k = 0; k <= i && k < a_width; k++)
		db->add_edge(cell, ID::A, k, ID::Y, i, -1);
}

void arith_binary_op(AbstractCellEdgesDatabase *db, RTLIL::Cell *cell)
{
	bool is_signed = cell->getParam(ID::A_SIGNED).as_bool();
	int a_width = GetSize(cell->getPort(ID::A));
	int b_width = GetSize(cell->getPort(ID::B));
	int y_width = GetSize(cell->getPort(ID::Y));

	if (!is_signed && cell->type != ID($sub)) {
		int ab_width = std::max(a_width, b_width);
		y_width = std::min(y_width, ab_width+1);
	}

	for (int i = 0; i < y_width; i++)
	{
		for (int k = 0; k <= i; k++)
		{
			if (k < a_width)
				db->add_edge(cell, ID::A, k, ID::Y, i, -1);

			if (k < b_width)
				db->add_edge(cell, ID::B, k, ID::Y, i, -1);
		}
	}
}

void reduce_op(AbstractCellEdgesDatabase *db, RTLIL::Cell *cell)
{
	int a_width = GetSize(cell->getPort(ID::A));

	for (int i = 0; i < a_width; i++)
		db->add_edge(cell, ID::A, i, ID::Y, 0, -1);
}

void compare_op(AbstractCellEdgesDatabase *db, RTLIL::Cell *cell)
{
	int a_width = GetSize(cell->getPort(ID::A));
	int b_width = GetSize(cell->getPort(ID::B));

	for (int i = 0; i < a_width; i++)
		db->add_edge(cell, ID::A, i, ID::Y, 0, -1);

	for (int i = 0; i < b_width; i++)
		db->add_edge(cell, ID::B, i, ID::Y, 0, -1);
}

void mux_op(AbstractCellEdgesDatabase *db, RTLIL::Cell *cell)
{
	int a_width = GetSize(cell->getPort(ID::A));
	int b_width = GetSize(cell->getPort(ID::B));
	int s_width = GetSize(cell->getPort(ID::S));

	for (int i = 0; i < a_width; i++)
	{
		db->add_edge(cell, ID::A, i, ID::Y, i, -1);

		for (int k = i; k < b_width; k += a_width)
			db->add_edge(cell, ID::B, k, ID::Y, i, -1);

		for (int k = 0; k < s_width; k++)
			db->add_edge(cell, ID::S, k, ID::Y, i, -1);
	}
}

void bmux_op(AbstractCellEdgesDatabase *db, RTLIL::Cell *cell)
{
	int width = GetSize(cell->getPort(ID::Y));
	int a_width = GetSize(cell->getPort(ID::A));
	int s_width = GetSize(cell->getPort(ID::S));

	for (int i = 0; i < width; i++)
	{
		for (int k = i; k < a_width; k += width)
			db->add_edge(cell, ID::A, k, ID::Y, i, -1);

		for (int k = 0; k < s_width; k++)
			db->add_edge(cell, ID::S, k, ID::Y, i, -1);
	}
}

void demux_op(AbstractCellEdgesDatabase *db, RTLIL::Cell *cell)
{
	int width = GetSize(cell->getPort(ID::Y));
	int a_width = GetSize(cell->getPort(ID::A));
	int s_width = GetSize(cell->getPort(ID::S));

	for (int i = 0; i < width; i++)
	{
		db->add_edge(cell, ID::A, i % a_width, ID::Y, i, -1);
		for (int k = 0; k < s_width; k++)
			db->add_edge(cell, ID::S, k, ID::Y, i, -1);
	}
}

PRIVATE_NAMESPACE_END

bool YOSYS_NAMESPACE_PREFIX AbstractCellEdgesDatabase::add_edges_from_cell(RTLIL::Cell *cell)
{
	if (cell->type.in(ID($not), ID($pos))) {
		bitwise_unary_op(this, cell);
		return true;
	}

	if (cell->type.in(ID($and), ID($or), ID($xor), ID($xnor))) {
		bitwise_binary_op(this, cell);
		return true;
	}

	if (cell->type == ID($neg)) {
		arith_neg_op(this, cell);
		return true;
	}

	if (cell->type.in(ID($add), ID($sub))) {
		arith_binary_op(this, cell);
		return true;
	}

	if (cell->type.in(ID($reduce_and), ID($reduce_or), ID($reduce_xor), ID($reduce_xnor), ID($reduce_bool), ID($logic_not))) {
		reduce_op(this, cell);
		return true;
	}

	// FIXME:
	// if (cell->type.in(ID($shl), ID($shr), ID($sshl), ID($sshr), ID($shift), ID($shiftx))) {
	// 	shift_op(this, cell);
	// 	return true;
	// }

	if (cell->type.in(ID($lt), ID($le), ID($eq), ID($ne), ID($eqx), ID($nex), ID($ge), ID($gt))) {
		compare_op(this, cell);
		return true;
	}

	if (cell->type.in(ID($mux), ID($pmux))) {
		mux_op(this, cell);
		return true;
	}

	if (cell->type == ID($bmux)) {
		bmux_op(this, cell);
		return true;
	}

	if (cell->type == ID($demux)) {
		demux_op(this, cell);
		return true;
	}

	// FIXME: $mul $div $mod $divfloor $modfloor $slice $concat
	// FIXME: $lut $sop $alu $lcu $macc $fa

	return false;
}

