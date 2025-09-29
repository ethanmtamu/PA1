/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Ethan Miller
	UIN: 634002241
	Date: 9/23/25
*/
#include "common.h"
#include "FIFORequestChannel.h"


using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1;
	int e = -1;
	int m = MAX_MESSAGE;
	bool inputFile = false;
	bool newchan = false;
	string sink = "";
	vector<FIFORequestChannel*> channels;

	
	string filename = "x1";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f': 
				filename = optarg;
				inputFile = true; 
				break;
			case 'c':
				newchan = true;
				break;
			case 'm':
				m = atoi (optarg);
				break;
		}
	}
	
	// Give arguments for server
	// Server needs './server', '-m', <value for -m arg>, 'NULL'
	// fork
	// In the child, run execvp using the server arguments

	std::string m_str = std::to_string(m);
	char* args[] = {(char*) "./server", (char*) "-m", (char*) m_str.c_str(), nullptr};
	pid_t pid = fork();
	if(pid == -1) {
		std::cerr << "Fork failed.\n";
		return 1;
	}
	else if(pid == 0) {
		execvp(args[0], args);	
		perror("Exec failed.");
		_exit(1);
	}



    FIFORequestChannel def_chan("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(&def_chan);

	if(newchan) {
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
		def_chan.cwrite(&nc, sizeof(MESSAGE_TYPE));
		// Create a variable to hold the name
		char* chname = new char[100];
		def_chan.cread(chname, 100);
		string str(chname);
		FIFORequestChannel* req_chan = new FIFORequestChannel(str, FIFORequestChannel::CLIENT_SIDE);
		channels.push_back(req_chan);
		
		delete[] chname;
	}	

	FIFORequestChannel chan = *(channels.back());

	// example data point request
	if(p != -1 && t != -1 && e != -1) {
		//Single data point if p, t, e =/= -1

		char buf[MAX_MESSAGE]; // 256
		datamsg x(p, t, e); // change from hardcoding to user's values
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan.cread(&reply, sizeof(double)); //answer

		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	else {
		// Else, if p =/= -1, do 1000 dp
		
		if(p != -1) {
			char buf[MAX_MESSAGE];
			ofstream file("received/" + filename + ".csv");
			for(double s_t = 0.0; s_t < 0.004*1000; s_t += 0.004) {
				//rq ecg1
				datamsg x(p, s_t, 1);
				memcpy(buf, &x, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg));
				double reply_1;
				chan.cread(&reply_1, sizeof(double));

				//rq ecg2
				datamsg y(p, s_t, 2);
				memcpy(buf, &y, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg));
				double reply_2;
				chan.cread(&reply_2, sizeof(double));

				file << s_t << "," << reply_1 << "," << reply_2 << "\n";
			}
			file.close();
		}
	}

	if(inputFile) {
		// sending a non-sense message, you need to change this
		filemsg fm(0, 0);
		string fname = filename;
		string oname = "received/" + filename;
		
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan.cwrite(buf2, len);  // I want the file length;

		char* buf3 = new char[m];

		int64_t filesize = 0;
		chan.cread(&filesize, sizeof(int64_t));
		cout << "Filesize is " << filesize << " bytes.";
		
		// Loop over the segments in file filesize / buff capacity
		int64_t iters = 0;
		ofstream file(oname, ios::binary);
		for(int64_t rem = filesize; rem > 0; rem -= m) {
			filemsg* filereq = (filemsg*)buf2;
			filereq->offset = iters * m;
			if(m < rem) {
				filereq->length = m;
			}
			else {
				filereq->length = rem;
			}
			chan.cwrite(buf2, len);

			// receive the response
			// cread into buf3, length file_req->len
			// write buf3 into file /received/filename

			chan.cread(buf3, filereq->length);
			file.write(buf3, filereq->length);
			
			iters++;
		}

		file.close();
		delete[] buf2;
		delete[] buf3;
	}

	if(newchan) {
		FIFORequestChannel* req_chan = channels.back();
	 	MESSAGE_TYPE qm = QUIT_MSG;
     	chan.cwrite(&qm, sizeof(MESSAGE_TYPE));
	 	delete req_chan;
	 	channels.pop_back();
		chan = *(channels.back());
	}

	// closing the channel    
    MESSAGE_TYPE qm = QUIT_MSG;
    chan.cwrite(&qm, sizeof(MESSAGE_TYPE));
	
}
