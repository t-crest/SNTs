#include "schedule.hpp"


router_id operator-(const router_id& lhs, const router_id& rhs) {
	router_id ret = {lhs.first-rhs.first, lhs.second-rhs.second};
	return ret;
}

router_id abs(const router_id& arg) {
	router_id ret = {abs(arg.first), abs(arg.second)};
	return ret;
}

std::ostream& operator<<(std::ostream& stream, const port_id& rhs) {
	static const std::map<port_id, char> m = {{N,'N'},{S,'S'},{E,'E'},{W,'W'},{L,'L'}};
	stream << m.at(rhs);
	return stream;
}

std::istream& operator>>(std::istream& stream, port_id& rhs) {
	static const std::map<char,port_id> m = {{'N',N},{'S',S},{'E',E},{'W',W},{'L',L}};
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



////////////////////////////////////////////////////////////////////////////////

/** Returns true if nothing has been scheduled at timeslot t */
bool schedule::available(timeslot t) {
	return ! util::contains(this->table, t);
}
/** Get the channel which is scheduled in timeslot t */
channel* schedule::get(timeslot t) {
	return this->table.at(t);
}
/** Returns the last timeslot where we have something scheduled */
timeslot schedule::max_time() {
	if (this->table.empty()) return 0;
	return (this->table.rbegin()->first); // rbegin is iterator to pair having the greatest key (and timeslot is the key)
}
/** Schedule channel c in timeslot t */
void schedule::add(channel *c, timeslot t) {
	this->table[t] = c;
}
/** Removes whatever channel is scheduled at t */
void schedule::remove(timeslot t) 
{
	this->table.erase(t);
}
/** Removes channel c from all timeslots */
void schedule::remove(channel *c) 
{
	auto it = this->table.begin();
	const auto end = this->table.end();

	while (it != end) {
		const bool remove = (it->second == c);
		if (remove)		this->table.erase(it++);
		else			++it;
	}
}

////////////////////////////////////////////////////////////////////////////////

link_t::link_t(port_out_t& _source, port_in_t& _sink) 
:	source(_source), sink(_sink), wrapped(false)
{
	const bool loop = (&this->source.parent == &this->sink.parent);
	warn_if(loop, "Self-loop detected on link");
	
	// Update the ports themselves, that they are infact connect to this link
	this->source.add_link(this);
	this->sink.add_link(this);
}

std::ostream& operator<<(std::ostream& stream, const link_t& rhs) {
	stream << rhs.source << "-->" << rhs.sink;
	return stream;
}

////////////////////////////////////////////////////////////////////////////////

port_t::port_t(router_t& _parent, port_id _corner) : l(NULL), parent(_parent), corner(_corner) {}
port_t::~port_t() {}
void port_t::add_link(link_t *l) {
	ensure(this->l==NULL, "Port " << *this << " already has a link connected: " << *(this->l));
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

port_out_t::port_out_t(router_t& _router, port_id _corner) : port_t(_router, _corner) {}
port_in_t::port_in_t(router_t& _router, port_id _corner) : port_t(_router, _corner) {}

////////////////////////////////////////////////////////////////////////////////

router_t::router_t(router_id _address) 
:	address(_address), ports_in(init<port_in_t>(this)), ports_out(init<port_out_t>(this))
{
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

////////////////////////////////////////////////////////////////////////////////

network_t::network_t(uint rows, uint cols) : m_routers(rows,cols) {}

uint network_t::rows() const {return this->m_routers.rows();} 
uint network_t::cols() const {return this->m_routers.cols();}

bool network_t::has(router_id r) {
	return (this->m_routers(r) != NULL);
}

link_t* network_t::add(port_out_t& source, port_in_t& sink) {
	link_t *l = new link_t(source, sink);
	this->link_ts.push_back(l);
	return l;
}

router_t* network_t::add(router_id r) {
	ensure(0 <= r.first && r.first <= this->cols()-1, "Tried to add router " << r << " outside network");
	ensure(0 <= r.second && r.second <= this->rows()-1, "Tried to add router " << r << " outside network");

	if (this->m_routers(r) == NULL) { // create only new router if it didn't exist before
		router_t *rr = new router_t(r); 
		this->m_routers(r) = rr;
		this->router_ts.push_back(rr);
		assert(this->router(r) == &this->router(r)->out(N).parent);
		assert(this->router(r) == &this->router(r)->out(L).parent);
	}
	return this->m_routers(r);
}

router_t* network_t::router(router_id r) {
	return this->m_routers(r);
}

const vector<link_t*>& network_t::links() const {
	return this->link_ts;
}

const vector<router_t*>& network_t::routers() const {
	return this->router_ts;
}

/**
 * Traverse the topology graph backwards from destination via BFS.
 * During BFS the depth of nodes is stored as the the number of hops.
 * This updates the .next map member variable of all nodes connected to dest.
 */
void network_t::shortest_path_bfs(router_t *dest) 
{
	std::queue<router_t*> Q; // the BFS queue

	// We don't want to pollute our router_t's class with ad-hoc members, 
	// instead we use std::maps to associate these properties 
	std::map<router_t*, bool> marked;
	std::map<router_t*, int> hops;
	
	Q.push(dest);
	marked[dest] = true;
	
	while (!Q.empty()) 
	{
		router_t *t = Q.front(); Q.pop(); 
		
//		cout << "Router " << t->address << " is " << hops[t] << " hops away from "<< dest->address << endl;
		
		for (int i = 0; i < __NUM_PORTS; i++) {
			if (!t->out((port_id)i).has_link()) continue;
			router_t *c = &t->out((port_id)i).link().sink.parent;
			if (!util::contains(hops, c)) continue;
			
			assert(hops[c] == hops[t]-1);
//			debugf(c->address);
//			debugf((port_id)i);
			
			t->next[dest->address].insert( &t->out((port_id)i) ); 
		}

		for (int i = 0; i < __NUM_PORTS; i++) {
			if (!t->in((port_id)i).has_link()) continue;
			router_t *o = &t->in((port_id)i).link().source.parent; // visit backwards
			
			if (!marked[o]) {
				marked[o] = true;
				hops[o] = hops[t]+1;
				Q.push(o);
			}
		}
	}
}

void network_t::shortest_path() {
	for_each(this->routers(), [this](router_t *r) {
		this->shortest_path_bfs(r);
	});
}

void network_t::print_next_table() {
	for_each(this->routers(), [&](router_t *r) {
	for_each(r->next, [&](const pair<router_id, set<port_out_t*> >& p) {
	for_each(p.second, [&](port_out_t *o) {
				cout << "From router " << r->address << " take " << o->corner << " towards " << p.first << endl;
			});
		});
	});
}


//void network_t::print_channel_specification() {
//    cout << "Channels to be scheduled:" << endl;
//    for_each(this->channels(), [&](channel *c) {
//        cout << "From router " << c->from << " to " << c->to << " using bandwidth " << c->bandwidth << endl;
//    });
//}