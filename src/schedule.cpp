//#include <limits>

#include "schedule.hpp"

router_id operator-(const router_id& lhs, const router_id& rhs) {
	router_id ret = {lhs.first - rhs.first, lhs.second - rhs.second};
	return ret;
}

router_id abs(const router_id& arg) {
	router_id ret = {abs(arg.first), abs(arg.second)};
	return ret;
}

std::ostream& operator<<(std::ostream& stream, const port_id& rhs) {
	static const std::map<port_id, char> m = {
		{N, 'N'},
		{S, 'S'},
		{E, 'E'},
		{W, 'W'},
		{L, 'L'}};
	stream << m.at(rhs);
	return stream;
}

std::istream& operator>>(std::istream& stream, port_id& rhs) {
	static const std::map<char, port_id> m = {
		{'N', N},
		{'S', S},
		{'E', E},
		{'W', W},
		{'L', L}};
	char c;
	stream >> c;
	rhs = m.at(c);
	return stream;
}

std::ostream& operator<<(std::ostream& stream, const routerport_id& rhs) {
	stream << rhs.first << rhs.second;
	return stream;
}

std::istream& operator>>(std::istream& stream, routerport_id& rhs) {
	stream >> rhs.first >> rhs.second;
	return stream;
}

std::ostream& operator<<(std::ostream& stream, const channel& rhs) {
    stream << "[from=" << rhs.from << ", to=" << rhs.to << ", bandwith=" << rhs.bandwidth << "]";
    return stream;
}

////////////////////////////////////////////////////////////////////////////////

schedule::schedule() 
#ifdef USE_SCHEDULE_HASHMAP
:	max(0) 
#endif
{}

#ifdef USE_SCHEDULE_HASHMAP
void schedule::refresh_max() {
	timeslot t = 0;
	for (auto it = this->table.cbegin(); it != this->table.cend(); ++it)
		t = util::max(t, it->first);
	this->max = t;
}
#endif

/** Returns true if nothing has been scheduled at timeslot t */
bool schedule::available(timeslot t) {
	return ! util::contains(this->table, t);
}

/** Returns true if something has been scheduled at timeslot t */
bool schedule::has(const timeslot t) {
	return util::contains(this->table, t);
}

bool schedule::is(const timeslot t, const channel* c){
	if(!this->has(t)) return false;
	if(this->get(t) != c) return false;
	return true;
}

/** Returns true if channel c is contained in the schedule */
boost::optional<timeslot> schedule::time(const channel *c) {
	boost::optional<timeslot> ret;
	for (auto it = this->table.begin(); it != this->table.end(); ++it) {
		if (it->second == c) return it->first;
	}
	return ret;
}

/** Get the channel which is scheduled in timeslot t */
const channel* schedule::get(const timeslot t) {
#ifdef USE_SCHEDULE_HASHMAP
	assert(t <= this->max);
#endif
	return this->table.at(t);
}

std::set<const channel*> schedule::channels() const {
	std::set<const channel*> ret;
	for(auto it = this->table.cbegin(); it != this->table.cend(); ++it)
		ret.insert(it->second);
	
	return ret;
}

/** Returns the last timeslot where we have something scheduled */
timeslot schedule::max_time() {
	if (this->table.empty()) return 0;

#ifdef USE_SCHEDULE_HASHMAP
	return this->max;
#else
	return (this->table.rbegin()->first); // rbegin is iterator to pair having the greatest key (and timeslot is the key)
#endif
}

/** Schedule channel c in timeslot t */
void schedule::add(const channel *c, timeslot t) {
	this->table[t] = c;

#ifdef USE_SCHEDULE_HASHMAP
	this->max = util::max(this->max, t);
#endif
}

/** Removes whatever channel is scheduled at t */
void schedule::remove(timeslot t) {
	this->table.erase(t);

#ifdef USE_SCHEDULE_HASHMAP
	this->refresh_max();
#endif
}

/** Removes channel c from all timeslots */
void schedule::remove(channel *c) {
	auto it = this->table.begin();
	const auto end = this->table.end();

	while (it != end) {
		const bool remove = (it->second == c);
		if (remove) this->table.erase(it++);
		else ++it;
	}

#ifdef USE_SCHEDULE_HASHMAP
	this->refresh_max();
#endif
}

void schedule::clear() {
#ifdef USE_SCHEDULE_HASHMAP
	this->max = 0;
#endif
	this->table.clear();
}

schedule& schedule::operator == (const schedule& rhs) {
#ifdef USE_SCHEDULE_HASHMAP
	this->max = rhs.max;
#endif
	this->table = rhs.table;
}

////////////////////////////////////////////////////////////////////////////////

link_t::link_t(port_out_t& _source, port_in_t& _sink)
: source(_source), sink(_sink), wrapped(false) {
	const bool loop = (&this->source.parent == &this->sink.parent);
	warn_if(loop, "Self-loop detected on link");

	// Update the ports themselves, that they are infact connect to this link
	this->source.add_link(this);
	this->sink.add_link(this);
}

link_t::link_t(port_out_t& _source, port_in_t& _sink, bool in)
: source(_source), sink(_sink), wrapped(false) {
	if(in){
		this->sink.add_link(this);
	} else {
		this->source.add_link(this);
	}
	
}

void link_t::updatebest() {
	this->best_schedule = this->local_schedule;
}

std::ostream& operator<<(std::ostream& stream, const link_t& rhs) {
	stream << rhs.source << "-->" << rhs.sink;
	return stream;
}

////////////////////////////////////////////////////////////////////////////////

port_t::port_t(router_t& _parent, port_id _corner) : l(NULL), parent(_parent), corner(_corner) {
}

port_t::~port_t() {
}

void port_t::add_link(link_t *l) {
	ensure(this->l == NULL, "Port " << *this << " already has a link connected: " << *(this->l));
	this->l = l; // now it's connected, but it shouldn't have been before
}

link_t& port_t::link() {
	assert(this->l);
	return *(this->l);
}

bool port_t::has_link() {
	return (this->l != NULL);
}

std::ostream& operator<<(std::ostream& stream, const port_t& rhs) {
	routerport_id rp_id = {rhs.parent.address, rhs.corner};
	stream << rp_id;
	return stream;
}

port_out_t::port_out_t(router_t& _router, port_id _corner) : port_t(_router, _corner) {
}

port_in_t::port_in_t(router_t& _router, port_id _corner) : port_t(_router, _corner) {
}

////////////////////////////////////////////////////////////////////////////////

router_t::router_t(router_id _address)
: address(_address), ports_in(init<port_in_t>(this)), ports_out(init<port_out_t>(this)){	
	// , local_in({this,L},{this,L},true), local_out({this,L},{this,L},false)
	//local_out = new link_t({this,L},{this,L},false);
	//local_in = new link_t({this,L},{this,L},true);
	assert(&this->in(N).parent == this);
	assert(&this->in(S).parent == this);
	assert(&this->in(E).parent == this);
	assert(&this->in(W).parent == this);
	assert(&this->in(L).parent == this);

	assert(&this->out(N).parent == this);
	assert(&this->out(S).parent == this);
	assert(&this->out(E).parent == this);
	assert(&this->out(W).parent == this);
	assert(&this->out(L).parent == this);
}

port_out_t& router_t::out(port_id p) {
	return this->ports_out[p];
}

port_in_t& router_t::in(port_id p) {
	return this->ports_in[p];
}

void router_t::updatebest() {
	this->local_in_best_schedule = this->local_in_schedule;
	this->local_out_best_schedule = this->local_out_schedule;
}
