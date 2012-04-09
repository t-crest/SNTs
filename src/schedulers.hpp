#ifndef SCHEDULERS_HPP
#define	SCHEDULERS_HPP

#include "output.hpp"
#include "draw.hpp"
#include "svg.h"
#include "parser.hpp"
#include "higher_order.h"
#include "lex_cast.h"
#include "util.hpp"
#include "matrix.hpp"
#include "schedule.hpp"
#include "file.hpp"
#include "draw.hpp"
#include "options.h"
#include <array>
#include <stack>
#include <queue>
#include <random>
#include <iostream>
#include <cstdlib>
#include <memory>

using namespace std;




class scheduler {
protected:
	network_t& n;
public:
	scheduler(network_t& _n);
	virtual void run() = 0;
};


class s_greedy : public scheduler {
	bool random;
public:
	s_greedy(network_t& _n, bool _random);
	void run();
};


class s_random : public scheduler {
public:
	s_random(network_t& _n);
	void run();
};


#endif	/* SCHEDULERS_HPP */

