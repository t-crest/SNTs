/*******************************************************************************
 * Copyright 2012 Mark Ruvald Pedersen <mark@radix63.dk>
 * Copyright 2012 Rasmus Bo Soerensen <rasmus@rbscloud.dk>
 * Copyright 2012 Jaspur Hoejgaard <jaspurh@gmail.com>
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 * 
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the copyright holder.
 ******************************************************************************/
 
/**
 * Generates a random graph to schedule onto the NoC
 */

#include "graph_generator.h"


graph_generator::graph_generator(int _n, float channel_factor, int bw_min, int bw_max, string type, string _output_dir)
:	n(_n), max_channels((_n*(_n-1))), output_dir(_output_dir)
{
	
	assert(n > 0);
	assert(channel_factor >= 0.0 && channel_factor <= 1.0);
	assert(bw_min >= 1);
	assert(bw_max >= bw_min);
	assert((type.compare("bitorus") != 0) || (type.compare("mesh") != 0));

//	cout << "assertions done" << endl;
	// Initialize
	pugi::xml_document doc;
	
	pugi::xml_node platform = doc.append_child("platform");
	platform.append_attribute("width").set_value(n);
	platform.append_attribute("height").set_value(n);
	
	pugi::xml_node topology = platform.append_child("topology");
	topology.append_attribute("type").set_value(type.c_str());

	srand(time(NULL));

//	cout << "init done" << endl;
	// Channels
	pugi::xml_node comm = doc.append_child("communication");
	comm.append_attribute("type").set_value("custom");
	
	for (int i = 0; i < n; i++) { // first coordinate from
		for (int j = 0; j < n; j++) { // second coordinate from
			for (int k = 0; k < n; k++) { // first coordinate to
				for (int l = 0; l < n; l++) { // second coordinate to
					if(i == k && j == l) // Don't schedule channel to same node
						continue;
					
					if (util::random_at_most(100) <= channel_factor*100) {
						char* from_node = new char[9];
						char* to_node = new char[9];
						pugi::xml_node channel = comm.append_child("channel");
						sprintf(from_node,"(%i,%i)",i,j);
						channel.append_attribute("from").set_value(from_node);
						sprintf(to_node,"(%i,%i)",k,l);
						channel.append_attribute("to").set_value(to_node);
						if (bw_max-bw_min == 0) {
							channel.append_attribute("bandwidth").set_value(bw_min);
						} else {
							channel.append_attribute("bandwidth").set_value((int)util::random_at_most(bw_max-bw_min) + bw_min);
						}
						
					}
				}
			}
			
		}
	}
//	cout << "loops done" << endl;
	
	sprintf(default_file_name,"%s%s%ix%i_cf%.2f_(%i_%i).xml",output_dir.c_str(),type.c_str(),n,n,channel_factor,bw_min,bw_max);
	doc.save_file(default_file_name);
	
//	cout << "file done" << endl;
}



graph_generator::~graph_generator() {
	
}