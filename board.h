#pragma once
#include <array>
#include <iostream>
#include <iomanip>
#include <vector>
/**
 * array-based board for 2048
 *
 * index (1-d form):
 *  (0)  (1)  (2)  (3)
 *  (4)  (5)  (6)  (7)
 *  (8)  (9) (10) (11)
 * (12) (13) (14) (15)
 *
 */
class board {
	static const std::vector<std::vector<int>> feature_indices;
public:
	typedef uint32_t cell;
	typedef std::array<cell, 4> row;
	typedef std::array<row, 4> grid;
	typedef uint64_t data;
	typedef int reward;
	int * index_to_tile;
	int * index_to_score;
public:
	board() : tile(), attr(0){ 
		build_table();
	}
	board(const grid& b, data v = 0) : tile(b), attr(v){
		build_table();
	}
	board(const board& b) = default;
	board& operator =(const board& b) = default;

	operator grid&() { return tile; }
	operator const grid&() const { return tile; }
	row& operator [](unsigned i) { return tile[i]; }
	const row& operator [](unsigned i) const { return tile[i]; }
	cell& operator ()(unsigned i) { return tile[i / 4][i % 4]; }
	const cell& operator ()(unsigned i) const { return tile[i / 4][i % 4]; }

	data info() const { return attr; }
	data info(data dat) { data old = attr; attr = dat; return old; }

public:
	bool operator ==(const board& b) const { return tile == b.tile; }
	bool operator < (const board& b) const { return tile <  b.tile; }
	bool operator !=(const board& b) const { return !(*this == b); }
	bool operator > (const board& b) const { return b < *this; }
	bool operator <=(const board& b) const { return !(b < *this); }
	bool operator >=(const board& b) const { return !(*this < b); }

public:
	
	void build_table(){
		index_to_tile = new int [16];
		index_to_score = new int [16];
		for(int i = 0;i<4;i++)
		{
			index_to_tile[i] = i;
			index_to_score[i] = 0;
		}
		index_to_score[3] = 3;
		for(int i = 4;i<16;i++){
			index_to_tile[i] = 2 * index_to_tile[i - 1];
			index_to_score[i] = 3 * index_to_score[i - 1];
		}
	}
	std::vector<int> features(){
		std::vector<int> indices;
		for(int i = 0 ;i<feature_indices.size();i++){
			int index = 0;
			for(int j = 0;j<feature_indices[i].size();j++){
				index *= 16;
				index += (*this)(feature_indices[i][j]);
				
			}
			indices.push_back(index);
		}
		return indices;
	}
	/**
	 * place a tile (index value) to the specific position (1-d form index)
	 * return 0 if the action is valid, or -1 if not
	 */
	reward place(unsigned pos, cell tile) {
		if (pos >= 16) return -1;
		if (tile != 1 && tile != 2 && tile != 3) return -1;
		operator()(pos) = tile;
		return 0;
	}

	/**
	 * apply an action to the board
	 * return the reward of the action, or -1 if the action is illegal
	 */
	reward slide(unsigned opcode) {
		switch (opcode & 0b11) {
		case 0: return slide_up();
		case 1: return slide_right();
		case 2: return slide_down();
		case 3: return slide_left();
		default: return -1;
		}
	}

	reward slide_left() {
		board prev = *this;
		reward score = 0;
		for (int r = 0; r < 4; r++) {
			std::vector<int> append_row(tile[r].begin(), tile[r].end());
			append_row.push_back(0);
			bool merge = false;
			int col = 0;
			while(append_row[0] && not merge){
				if (append_row[0] == append_row[1] && append_row[0] > 2){
					append_row.erase(append_row.begin());
					append_row[0] += 1;
					merge = true;
					score += index_to_score[append_row[0]];
				}else if(append_row[0] > 0 && append_row[1] > 0 && append_row [0] + append_row[1] == 3){
					append_row.erase(append_row.begin());
					append_row[0] = 3;
					score += index_to_score[append_row[0]];
					merge = true;
				}
				tile[r][col] = append_row[0];
				col += 1;
				if(! merge){
					append_row.erase(append_row.begin());
				}
			}
			for(int i = 0; i< append_row.size() - 1 ;i++){
				tile[r][col+i] = append_row[i + 1];
			}
		}
		return (*this != prev) ? score : -1;
	}
	reward slide_right() {
		reflect_horizontal();
		reward score = slide_left();
		reflect_horizontal();
		return score;
	}
	reward slide_up() {
		rotate_right();
		reward score = slide_right();
		rotate_left();
		return score;
	}
	reward slide_down() {
		rotate_right();
		reward score = slide_left();
		rotate_left();
		return score;
	}

	void transpose() {
		for (int r = 0; r < 4; r++) {
			for (int c = r + 1; c < 4; c++) {
				std::swap(tile[r][c], tile[c][r]);
			}
		}
	}

	void reflect_horizontal() {
		for (int r = 0; r < 4; r++) {
			std::swap(tile[r][0], tile[r][3]);
			std::swap(tile[r][1], tile[r][2]);
		}
	}

	void reflect_vertical() {
		for (int c = 0; c < 4; c++) {
			std::swap(tile[0][c], tile[3][c]);
			std::swap(tile[1][c], tile[2][c]);
		}
	}

	/**
	 * rotate the board clockwise by given times
	 */
	void rotate(int r = 1) {
		switch (((r % 4) + 4) % 4) {
		default:
		case 0: break;
		case 1: rotate_right(); break;
		case 2: reverse(); break;
		case 3: rotate_left(); break;
		}
	}

	void rotate_right() { transpose(); reflect_horizontal(); } // clockwise
	void rotate_left() { transpose(); reflect_vertical(); } // counterclockwise
	void reverse() { reflect_horizontal(); reflect_vertical(); }
	
public:
	friend std::ostream& operator <<(std::ostream& out, const board& b) {
		out << "+------------------------+" << std::endl;
		for (auto& row : b.tile) {
			out << "|" << std::dec;
			for (auto t : row) out << std::setw(6) << b.index_to_tile[t];
			out << "|" << std::endl;
		}
		out << "+------------------------+" << std::endl;
		return out;
	}
	
private:
	grid tile;
	data attr;
};
const std::vector<std::vector<int>> board::feature_indices({{0,1,2,3},{4,5,6,7},{8,9,10,11},{12,13,14,15},{0,4,8,12},{1,5,9,13},{2,6,10,14},{3,7,11,15}});
