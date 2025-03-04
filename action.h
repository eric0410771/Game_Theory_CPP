#pragma once
#include <algorithm>
#include <unordered_map>
#include <string>
#include "board.h"

class action {
public:
	action(unsigned code = -1u) : code(code){}
	action(const action& a) : code(a.code) {}
	virtual ~action() {}

	class slide; // create a sliding action with board opcode
	class place; // create a placing action with position and tile

public:
	virtual board::reward apply(board& b) const {
		auto proto = entries().find(type());
		if (proto != entries().end()) return proto->second->reinterpret(this).apply(b);
		return -1;
	}
	virtual std::ostream& operator >>(std::ostream& out) const {
		auto proto = entries().find(type());
		if (proto != entries().end()) return proto->second->reinterpret(this) >> out;
		return out << "??";
	}
	virtual std::istream& operator <<(std::istream& in) {
		auto state = in.rdstate();
		for (auto proto = entries().begin(); proto != entries().end(); proto++) {
			if (proto->second->reinterpret(this) << in) return in;
			in.clear(state);
		}
		return in.ignore(2);
	}

public:
	operator unsigned() const { return code; }
	unsigned type() const { return code & type_flag(-1u); }
	unsigned event() const { return code & ~type(); }
	friend std::ostream& operator <<(std::ostream& out, const action& a) { return a >> out; }
	friend std::istream& operator >>(std::istream& in, action& a) { return a << in; }

protected:
	static constexpr unsigned type_flag(unsigned v) { return v << 24; }

	typedef std::unordered_map<unsigned, action*> prototype;
	static prototype& entries() { static prototype m; return m; }
	virtual action& reinterpret(const action* a) const { return *new (const_cast<action*>(a)) action(*a); }
	unsigned code;
};

class action::slide : public action {
public:
	static constexpr unsigned type = type_flag('s');
	slide(unsigned oper) : action(slide::type | (oper & 0b11)) {}
	slide(const action& a = {}) : action(a) {}
public:
	board::reward apply(board& b) const {
		return b.slide(event());
	}
	std::ostream& operator >>(std::ostream& out) const {
		return out << '#' << ("URDL")[event() & 0b11];
	}
	std::istream& operator <<(std::istream& in) {
		if (in.peek() == '#') {
			char v;
			in.ignore(1) >> v;
			const char* opc = "URDL";
			unsigned oper = std::find(opc, opc + 4, v) - opc;
			if (oper < 4) {
				operator= (action::slide(oper));
				return in;
			}
		}
		in.setstate(std::ios::failbit);
		return in;
	}
protected:
	action& reinterpret(const action* a) const { return *new (const_cast<action*>(a)) slide(*a); }
	static __attribute__((constructor)) void init() { entries()[type_flag('s')] = new slide; }
};

class action::place : public action {
public:
	static constexpr unsigned type = type_flag('p');
	place(unsigned pos, unsigned tile) : action(place::type | (pos & 0x0f) | (std::min(tile, 35u) << 4)) {}
	place(const action& a = {}) : action(a) {}
	unsigned position() const { return event() & 0x0f; }
	unsigned tile() const { return event() >> 4; }
public:
	board::reward apply(board& b) const {
		return b.place(position(), tile());
	}
	std::ostream& operator >>(std::ostream& out) const {
		const char* idx = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ?";
		return out << idx[position()] << idx[std::min(tile(), 36u)];
	}
	std::istream& operator <<(std::istream& in) {
		const char* idx = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		char v = in.peek();
		unsigned pos = std::find(idx, idx + 16, v) - idx;
		if (pos < 16) {
			in.ignore(1) >> v;
			unsigned tile = std::find(idx, idx + 36, v) - idx;
			if (tile < 36) {
				operator =(action::place(pos, tile));
				return in;
			}
		}
		in.setstate(std::ios::failbit);
		return in;
	}
protected:
	action& reinterpret(const action* a) const { return *new (const_cast<action*>(a)) place(*a); }
	static __attribute__((constructor)) void init() { entries()[type_flag('p')] = new place; }
};
