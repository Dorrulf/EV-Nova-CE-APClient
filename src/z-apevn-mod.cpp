#include <windows.h>
#include <stdio.h>
#include <shlobj.h>
#include "macros/patch.h"
#include "nova.h"
#include <stdlib.h>
#include "Archipelago.h"
#include <string>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <sstream>

// prototype my function so writeArrayToFile understands what it is when called.
//void writeAdditionals(const FILE *oFile, const int *iArray);

// New plan:
//	Intercept calls to the primary function doing the bit swapping (thankfully, only found 2 references)
//	Forward the call to let the function do its thing
//	Iterate over the region we care about and compare to see what changed
//	Interact based on that.
// Problem with old plan:
//	The primary function does way too much logic to determine 1. Will it change a bit? 2. What is the offset to that bit.
//	I was unable to figure out a way to capture when that bit was determined and actually being flipped without trying to reimplement the entire function with incomplete code...

/*
namespace patch {
    template <typename T>
    std::string to_string(const T& n) {
        std::ostringstream stm;
        stm << n;
        return stm.str();
    }
}
*/

extern "C" {

// void doAPInit(); //I think this isn't prototyping the way I want...

// Our source of "truth" about the state of the control bits.
// I would like to initialize it with the current loaded values (ex: reloading a pilot), but that would require hooking in elsewhere. Back to the idea of the menu hotkeys prompting the user for IP to connect? This could be part of that process.
// If the game is closed and reopenned, thus resetting this, the game *might* emit the same location again. But let's see how the server handles that before we try and bother with caching ourselves.
// We'll also have to see if the server cares that we sent a check id for a location it doesn't have...
//unsigned char truth_cbs[10000]; //as this is a global declaration, c should initialize the indexes... EVN also 0 indexes their bits
//int startingOffset = 2000;	//We care about control bits 2000 - 2999
//int *dynamicOffset = (int*)0x005914cc;	//this is where EVN stores the dynamic memory offset for the 10000 control bit range
int* dynamicOffset = (int*)0x005914cc;	//this is where EVN stores the dynamic memory offset for the 10000 control bit range
//size_t mySize = sizeof(truth_cbs)/sizeof(truth_cbs[0]);

char ini_path[] = ".\\ap_config.ini";

// looks like c naming convention is _, so this should prob be renamed to is_ap_init
bool isAPInit = false;

// filter file
char filterFilePath[] = ".\\aplocids.txt";
// filter map
std::unordered_map<int, int> emittableBits;

//FILE *file;

// credit offset?: *(int *)(DAT_005912a0 + 0xa0)
//int* credDynamicOffset = (int*)0x00591340;
int pDynamicOffset = 0x005912a0;
int credOffset = 0xa0;

int Credits1 = 9900;
int Credits5 = 9901;
int Credits10 = 9902;
int Credits50 = 9903;
int Credits100 = 9904;
int Credits500 = 9905;

// EVN - before AP
//void notifyPlr(std::string message) {
//void notifyPlr(const char* message) {
void notifyPlr(char* message) {
	//TODO: create message in bottom left UI where the dates update on each jump.
	//nv_ShowAlert(message);	//This may be causing a lot of crashes...
	std::cout << "Notify Player: " << message << std::endl;
}

// Sets the given bit. Does NOT care about Received/Emit - check that elsewhere.
void setBit(int bitOffset) {
	std::cout << "set bit called for bitOffset: " << bitOffset << std::endl;
	
	// Handle special cases
	if (bitOffset >= Credits1 && bitOffset <= Credits500) {
		//TODO: give credits.
		std::cout << "Credits received instead " << std::endl;
		int giveAmount = 0;
		if (bitOffset == Credits1) {
			giveAmount = 10000;
		} else if (bitOffset == Credits5) {
			giveAmount = 50000;
		} else if (bitOffset == Credits10) {
			giveAmount = 100000;
		} else if (bitOffset == Credits50) {
			giveAmount = 500000;
		} else if (bitOffset == Credits100) {
			giveAmount = 1000000;
		} else if (bitOffset == Credits500) {
			giveAmount = 5000000;
		} else {
			giveAmount = 75000; //we should never hit this
		}
		//std::cout << "Current credits: " << *credDynamicOffset << std::endl;
		//*credDynamicOffset = *credDynamicOffset + giveAmount;
		//int newCreds = *credDynamicOffset + giveAmount;	//just in case above statement didn't work as intended in c++
		//*credDynamicOffset = newCreds;
		int pMem = *(int *)(pDynamicOffset);
		//std::cout << "p mem offset: " << pMem << std::endl;
		int* credMem = (int *)(pMem + credOffset);
		//std::cout << "cred mem: " << credMem << " and cred val: " << *credMem << std::endl;
		int curCreds = *credMem;
		curCreds += giveAmount;
		*credMem = curCreds;
		
		std::cout << "new credits: " << curCreds << std::endl;
		
		return;
	}
	//int bitMem = *dynamicOffset + startingOffset + (short)i;	//adding our offset
	//unsigned char* bitMem = *dynamicOffset + startingOffset + (short)i;	//adding our offset
	//unsigned char* bitMem = *dynamicOffset + bitOffset;	//the offset should actually be baked into the server data, thus the passed int is already adjusted.
	//*bitOffset = 0x01;	//dereference the pointer based on our offset to get the value at that memory byte. Set it to 1.
	int* bitMem = reinterpret_cast<int*>(*dynamicOffset + bitOffset);
	
	//suppose we try a bit of safety?
	std::cout << "current bit val: " << *bitMem << std::endl;
	if (*bitMem == 0) {
		*bitMem = 1;
		std::cout << "new bit val: " << *bitMem << std::endl;
	}
	
	/*//void* addr = &(*dynamicOffset + bitOffset);	//generic pointer to memory address
	void* addr = &(bitMem);	//generic pointer to memory address
	uint8_t* byte_ptr = static_cast<uint8_t*>(addr); //convert to 8bit pointer using a type safe declaration
	*byte_ptr = 1	//set the value
	*/
	
	return;
}

bool loadFilterFile() {
	std::ifstream file(filterFilePath);
	std::string line;
	
	if (!file.is_open()) {
		std::cerr << "Could not open file: " << filterFilePath << std::endl;
        return false;
	}
	
	while (std::getline(file, line)) {
        std::stringstream ss(line);
        int key;

        // Parse the key from the line
        if (ss >> key) {
            // Insert into the unordered_map
            emittableBits[key] = 0; //set to evn's unset state
        } else {
            std::cerr << "Warning: Skipping invalid line format: " << line << std::endl;
        }
    }
    return true;
}


/* ------- AP Zone ----------*/
//	Functions related to communicating with AP server

void checkMessages() {
	
	while (AP_IsMessagePending()) {
		AP_Message myMsg = *AP_GetLatestMessage();
		
		char* msgChar = new char[myMsg.text.length() + 1];
		strcpy(msgChar, myMsg.text.c_str());
		std::cout << myMsg.text.c_str() << std::endl;
		AP_ClearLatestMessage();
		
		notifyPlr(msgChar);
	}
	
	return;
}


// define a callback function
auto clrCallback = []() {
	// I'm unsure of the context "clear" here. Action where I should reset progress (bits all to 0), or more like release / complete and flip all bits set to 1?
	//	TODO: Try releasing the game in console and see if this function get's called I guess.
	
	std::cout << "AP item clear " << std::endl;
	
	checkMessages();
	
	return;
};

// define a callback function
// Item Received
auto irCallback = [](int64_t id, bool notifyPlr) {
	// Things like:
	//	Ships
	//	Outfits
	//	Credits
	
	if (id > SHRT_MAX || id < SHRT_MIN) {
		std::cout << "!IMPORTANT: ID was out of short range. Conversion done with data loss." << id << std::endl;
	}
	
	// Let's convert the id to something we can use here
	//short my_short = (short)id;	// possible data loss, but should convert. Do we want to test if the ID was out of range?
	
	// For now, there is no real dif between the two cases recieved by Nova. All boils down to control bits.
	//	Except for credits.
	//	TODO: IDs for credits (ex: 9999 = 50k credits, 10000 = 500k credits)
	//		These will alter a different set of memory, so going to save for later.
	// else, flip bit
	//setBit(my_short);
	setBit((int)id);
	
	// TODO: If notifyPlr, add message string to player UI (where the date is updated after a jump)
	
	std::cout << "AP item received: " << id << std::endl;
	
	checkMessages();
	
	return;
};

// define a callback function
// Location Checked
auto lcCallback = [](int64_t id) {
	// I think some kind of confirmation is sent back when we emit, but I'm really not sure what's the purpose of this function in APCpp.
	
	if (id > SHRT_MAX || id < SHRT_MIN) {
		std::cout << "!IMPORTANT: ID was out of short range. Conversion done with data loss." << id << std::endl;
	}
	
	//setBit((int)id);
	
	std::cout << "AP location callback - SKIPPING LC BIT SETTING FOR NOW: " << id << std::endl;
	
	checkMessages();
	
	return;
};


void doAPInit(){
	//TODO: Can't have these be hard baked... Gotta get them from a config file or from the user or something.
	//AP_Init("archipelago.gg:52885", "EVN", "Dorrulf", "");
	char conn_str[256];
	DWORD result = GetPrivateProfileStringA("AP EVN", "conn_addr", "", conn_str, sizeof(conn_str), ini_path);
	if (result == 0) {
		std::cerr << "AP CONFIG ERROR: Did not find IP address" << std::endl;
	}
	
	char game_name[64]; //keep username under 64 characters
	result = GetPrivateProfileStringA("AP EVN", "game_name", "EV Nova", game_name, sizeof(game_name), ini_path);
	if (result == 0) {
		std::cerr << "AP CONFIG ERROR: Did not find valid game name" << std::endl;
	}
	
	char username[64]; //keep username under 64 characters
	result = GetPrivateProfileStringA("AP EVN", "username", "", username, sizeof(username), ini_path);
	if (result == 0) {
		std::cerr << "AP CONFIG ERROR: Did not find valid username" << std::endl;
	}
	
	char password[64]; //keep username under 64 characters
	result = GetPrivateProfileStringA("AP EVN", "password", "", password, sizeof(password), ini_path);
	if (result == 0) {
		std::cerr << "AP CONFIG ERROR: Did not find valid password" << std::endl;
	}
	
	AP_Init(conn_str, game_name, username, password);
	
	AP_SetItemClearCallback(clrCallback);
	AP_SetItemRecvCallback(irCallback);
	AP_SetLocationCheckedCallback(lcCallback);
	
	AP_Start();
	
	checkMessages();
	
	return;
}

/* ------- END --------------*/



/* ------- Nova Zone --------*/
// Functions more directly related to the logic of bit handling

void emitBit(int bitOffset) {
	std::cout << "AP sending val: " << bitOffset << std::endl;
	
	// Perform actual setstate
	// This may be overkill because we make one call that has essentially checked this, but just in case this is called from elsewhere:
	if (emittableBits.count(bitOffset) == 0) {
		std::cout << "Not allowed to emit bit: " << bitOffset << std::endl;
		return;
	}
	
	// NOTE: Currently are not populating this from a saved state post crash / exit
	//	so this will NOT perfectly prevent duplicate sends, which is why we double check below.
	if (emittableBits[bitOffset] == 1) {
		std::cout << "Already sent bit, thus skip: " << bitOffset << std::endl;
	}
	
	// Set our truth, then send to AP
	emittableBits[bitOffset] = 1;
	AP_SendItem(bitOffset);
	
	return;	
}

/* ------- END --------------*/

//TODO: Close file handlers, AP_Shutdown(), etc...



//int callLimit = 3;
//int callsMade = 0;

// intercept the original call
CALL(0x0044803f, _checkBits);
CALL(0x0044807a, _checkBits);
// the original function returns int vals between -1 and -4...

void checkBits() {
	// first, reissue the original call so it can process and flip the bits
	((void (*)())0x00449370)();
		
	//todo: move this down or out later, don't need to initialize file first usually
	// Create a file pointer
    //FILE *file;
	
	AP_ConnectionStatus APStatus = AP_GetConnectionStatus();
	
	if (!isAPInit) {		
		freopen("stdout.txt", "a", stdout);
		freopen("stderr.txt", "a", stderr);
		
		loadFilterFile();
		
		std::cout << "attempt to init AP" << std::endl;
		try {
			doAPInit();
		}
		catch (const std::exception& e) {
			std::cerr << "init exception: " << e.what() << std::endl;
		}
		catch (...) {
			std::cerr << "unknown error" << std::endl;
		}
		
		APStatus = AP_GetConnectionStatus();
		
		std::cout << "AP init: " << (int)APStatus << std::endl;
		//isAPInit = true;
		isAPInit = AP_IsInit();
	}
	
	// NOTE: The reissue above should let the game naturally set the bit, so let's not worry about setting it again.
	// Emit the bit flag IF it is part of the list AP server cares about.
	for (const auto& pair : emittableBits) {
		if (pair.second == 1) {
			continue;
		}
		
		int myKey = pair.first;
		
		int* bitMem = reinterpret_cast<int*>(*dynamicOffset + myKey);
		unsigned char cbit = *bitMem;
		
		if (cbit > 0) {
			//let the emit function set the value of emittableBits - in case it is called elsewhere.
			emitBit(myKey);
		}
		
	}
			
	/*if (APStatus == AP_ConnectionStatus::Disconnected || APStatus == AP_ConnectionStatus::ConnectionRefused || callsMade >= callLimit) {
		AP_Shutdown();
		fprintf(file, "closing AP: %d \n", APStatus);
		fclose(file);
		fclose(stdout);
		//fclose(stderr);
	}*/
	
	checkMessages();
	
    // Close the file
    //fclose(file);
	
	return;
}

}
