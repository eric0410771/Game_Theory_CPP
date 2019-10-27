#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"
#include "weight.h"

struct action_op{
	action s;
	int op;
};

typedef action_op action_op;

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b) { return action(); }
	virtual action take_action(const board& b, int last_slide){ return action();}
	virtual action_op take_action2(const board& b){
		action_op s;
		return s;
	;}
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};

/**
 * random environment
 * add a new random tile to an empty cell
 * 2-tile: 90%
 * 4-tile: 10%
 */
class rndenv : public random_agent {
public:
	rndenv(const std::string& args = "") : random_agent("name=random role=environment " + args),
		space({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }),popup(0,9) {
		initial_bag();
		}
	virtual action take_action(const board& after){
		std::shuffle(space.begin(), space.end(), engine);
                for (int pos : space) {
                        if (after(pos) != 0) continue;
                        board::cell tile = popup(engine) ? 1 : 2;
                        return action::place(pos, tile);
                }
                return action();
	}
	virtual action take_action(const board& after, int player_slide) {
		std::shuffle(space.begin(), space.end(), engine);
		std::vector<int> empty;

		if (player_slide != -1){
			//printf("player slide %d",player_slide);
			if (player_slide == 0){
				for(int i = 12 ;i < 16 ;i++){
					if(after(i)==0)
						empty.push_back(i);
				}
			}else if(player_slide == 1){
				for(int i = 0 ;i<4;i++){
					if(after(i*4)== 0)
						empty.push_back(i * 4);
				}
			}else if(player_slide == 2){
				for(int i = 0;i<4;i++){
					if(after(i) == 0){
						empty.push_back(i);
					}
				}
			}else if (player_slide == 3){
				for(int i = 0 ;i<4;i++){
					if(after(i * 4 + 3) == 0){
						empty.push_back(4 * i + 3);
					}
				}
			}
		}else{
			for(int i = 0;i<16;i++){
				if(after(i) == 0){
					empty.push_back(i);
				}
			}
		}
		if (empty.size() != 0){
			std::shuffle(empty.begin(), empty.end(), engine);
			int pos = empty[0];
			board::cell tile = choose_tile();
			return action::place(pos, tile);
		}
		return action();
	}
	void initial_bag(){
		std::vector<int> new_bag;
		bag = new_bag;
		bag.push_back(1);
		bag.push_back(2);
		bag.push_back(3);
	}
	board::cell choose_tile(){
		if (bag.size()==0)
			initial_bag();
		std::shuffle(bag.begin(), bag.end(),engine);		
		board::cell tile = bag[0];
		bag.erase(bag.begin());
		return tile;
	}
	virtual void open_episode(const std::string& flag = "") {initial_bag();}


private:
	std::array<int, 16> space;
	std::vector<int> bag;
	std::uniform_int_distribution<int> popup;
};

/**
 * dummy player
 * select a legal action randomly
 */
class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=dummy role=player " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action_op take_action2(const board& before) {
		std::shuffle(opcode.begin(), opcode.end(), engine);
		action_op output;
		for (int op : opcode) {
			board::reward reward = board(before).slide(op);
			action s = action::slide(op);
			output.s = s;
			output.op = op;
			if (reward != -1) return output;
		}
		output.s = action();
		output.op = -1;
		return output;
	}
	virtual action take_action(const board& b, int last_slide){
		return action();
	}

protected:
	std::array<int, 4> opcode;
};

class weight_agent: public player{
public:
	weight_agent(const std::string& args =  "", int num_feature = 8): player(args), num_feature(num_feature){
		alpha = 1.0 / 32.0;
		if (meta.find("init") != meta.end()){
			init_weights(meta["init"]);
		}
		if (meta.find("load") != meta.end()){
			load_weights(meta["load"]);
		}
	}
	virtual ~weight_agent(){
		if (meta.find("save")!=meta.end()){
			save_weights(meta["save"]);
		}
	}

protected:
	virtual void init_weights(const std::string& info){
		for (int i = 0 ;i<num_feature; i++){
			net.emplace_back(65536);
		}
	}

	virtual void load_weights(const std::string& path){
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if(!in.is_open()) std::exit(-1);

		uint32_t size;
		in.read(reinterpret_cast<char*>(&size),sizeof(size));
		net.resize(size);
		for (weight& w : net) in >> w;	
		in.close();
	}
	virtual void save_weights(const std:: string& path){
		std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if(!out.is_open()) std::exit(-1);
		uint32_t size;
		out.write(reinterpret_cast<char*>(&size),sizeof(size));
		for (weight& w : net) out << w;
		out.close();			
	}
	/*
	virtual action_op take_action2(const board& before) {
                std::shuffle(opcode.begin(), opcode.end(), engine);
                action_op output;
                for (int op : opcode) {
                        board::reward reward = board(before).slide(op);
                        action s = action::slide(op);
                        output.s = s;
                        output.op = op;
                        if (reward != -1) return output;
                }
                output.s = action();
                output.op = -1;
                return output;
        }*/
	virtual action_op take_action2(const board& before){
		int rewards[] = {0,0,0,0};
		float a_value[] = {0.0,0.0,0.0,0.0};
		int max = 0;
		int index = 0;
		bool first = true;
		action_op output;
		for(int i = 0 ;i <4;i++){
			board tmp = board(before);
			board::reward reward = tmp.slide(opcode[i]);
			
			rewards[i] = reward;
			a_value[i] = sum(tmp.features());
			if(reward!=-1){
				if(first){
					max = rewards[i]+a_value[i];
					index = i;
					first = false;
				}else{
					if(max < rewards[i]+a_value[i]){
						max = rewards[i]+a_value[i];
						index = i;
					}
				}
			}
		}
		output.s = action::slide(opcode[index]);
		output.op = opcode[index];
		return output;
	}
public:
	void update_weights(std::vector<std::vector<int>> state_index, std::vector<int> rewards, std::vector<std::vector<int>> after_state_index){
		/*float delta = alpha * (0 - sum(state_index[state_index.size()-1]));
		for (int j = 0 ; j<state_index[state_index.size()-1].size() ; j++){
			net[j][state_index[state_index.size()-1][j]] += delta;
		
		}
		for(int i = state_index.size() - 2; i >= 0; i--){
			delta = alpha * (rewards[i] + sum(after_state_index[i]) - sum(state_index[i]));
			for(int j = 0 ; j<state_index[i].size() ; j++){
				net[j][state_index[i][j]] += delta;
			}
		}
		*/
		float delta = 0;
		for (int i = 0 ;i<state_index.size();i++){
			if(rewards[i] != -1){
				delta = alpha*(rewards[i] + sum(after_state_index[i]) - sum(state_index[i]));
			}
			else{
				delta = alpha*(0 - sum(state_index[i]));
			}
			for(int j = 0;j<state_index[i].size();j++){
				net[j][state_index[i][j]] += delta;
			}
		}
	}
	float sum(std::vector<int> weight_index){
		float advantage_value = 0;
		for(int i = 0 ;i<weight_index.size();i++){
			advantage_value += net[i][weight_index[i]];
		}
		return advantage_value;
	}
protected:
	std::vector<weight> net;
private:
	int num_feature;
	float alpha;
};
