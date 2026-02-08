#include <windows.h>
#include <stdio.h>
#include <shlobj.h>
#include "macros/patch.h"
#include "nova.h"
#include <stdlib.h>
#include "Archipelago.h"
#include <string>
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
unsigned char truth_cbs[10000]; //as this is a global declaration, c should initialize the indexes... EVN also 0 indexes their bits
//int startingOffset = 2000;	//We care about control bits 2000 - 2999
//int *dynamicOffset = (int*)0x005914cc;	//this is where EVN stores the dynamic memory offset for the 10000 control bit range
int* dynamicOffset = (int*)0x005914cc;	//this is where EVN stores the dynamic memory offset for the 10000 control bit range
size_t mySize = sizeof(truth_cbs)/sizeof(truth_cbs[0]);

char ini_path[] = ".\\ap_config.ini";

// looks like c naming convention is _, so this should prob be renamed to is_ap_init
bool isAPInit = false;

FILE *file;

// EVN - before AP
//void notifyPlr(std::string message) {
//void notifyPlr(const char* message) {
void notifyPlr(char* message) {
	//TODO: create message in bottom left UI where the dates update on each jump.
	nv_ShowAlert(message);
}

void setBit(short bitOffset) {
	//int bitMem = *dynamicOffset + startingOffset + (short)i;	//adding our offset
	//unsigned char* bitMem = *dynamicOffset + startingOffset + (short)i;	//adding our offset
	//unsigned char* bitMem = *dynamicOffset + bitOffset;	//the offset should actually be baked into the server data, thus the passed int is already adjusted.
	//*bitOffset = 0x01;	//dereference the pointer based on our offset to get the value at that memory byte. Set it to 1.
	int* bitMem = reinterpret_cast<int*>(*dynamicOffset + bitOffset);
	*bitMem = 1;
	
	/*//void* addr = &(*dynamicOffset + bitOffset);	//generic pointer to memory address
	void* addr = &(bitMem);	//generic pointer to memory address
	uint8_t* byte_ptr = static_cast<uint8_t*>(addr); //convert to 8bit pointer using a type safe declaration
	*byte_ptr = 1	//set the value
	*/
	
	fprintf(file, "set bit: %d \n", bitOffset);
	
	return;
}


/* ------- AP Zone ----------*/
//	Functions related to communicating with AP server

void checkMessages() {
	
	while (AP_IsMessagePending()) {
		AP_Message myMsg = *AP_GetLatestMessage();
		//fprintf(file, "%s \n", myMsg.text.c_str());
		//msgStr = std::to_string(myMsg.text);
		//std::string msgStr = patch::to_string(myMsg.text);
		//msgStr = myMsg.text; //is already std::string.
		//need it to be mutable though I guess... c_str returns const, which caused a problem.
		//const char* msgStr = myMsg.text.c_str();
		char* msgChar = new char[myMsg.text.length() + 1];
		strcpy(msgChar, myMsg.text.c_str());
		fprintf(file, "%s \n", myMsg.text.c_str());
		AP_ClearLatestMessage();
		
		//notifyPlr(msgStr);
		notifyPlr(msgChar);
	}
	
	return;
}


// define a callback function
auto clrCallback = []() {
	if (file == NULL) {
		return;
	}
	
	// I'm unsure of the context "clear" here. Action where I should reset progress (bits all to 0), or more like release / complete and flip all bits set to 1?
	//	TODO: Try releasing the game in console and see if this function get's called I guess.
	
	fprintf(file, "AP item clear \n");
	
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
	if (file == NULL) {
		return;
	}
	
	if (id > SHRT_MAX || id < SHRT_MIN) {
		fprintf(file, "!IMPORTANT: ID was out of short range. Conversion done with data loss.");
	}
	
	// Let's convert the id to something we can use here
	short my_short = (short)id;	// possible data loss, but should convert. Do we want to test if the ID was out of range?
	
	// For now, there is no real dif between the two cases recieved by Nova. All boils down to control bits.
	//	Except for credits.
	//	TODO: IDs for credits (ex: 9999 = 50k credits, 10000 = 500k credits)
	//		These will alter a different set of memory, so going to save for later.
	// else, flip bit
	setBit(my_short);
	
	// TODO: If notifyPlr, add message string to player UI (where the date is updated after a jump)
	
	fprintf(file, "AP item received: %lld %d \n", id, notifyPlr);
	
	checkMessages();
	
	return;
};

// define a callback function
// Location Checked
auto lcCallback = [](int64_t id) {
	// Things like:
	//	Missions
	//	Systems?
	//	Spobs?
	if (file == NULL) {
		return;
	}
	
	if (id > SHRT_MAX || id < SHRT_MIN) {
		fprintf(file, "!IMPORTANT: ID was out of short range. Conversion done with data loss.");
	}
	
	// Let's convert the id to something we can use here
	short my_short = (short)id;	// possible data loss, but should convert. Do we want to test if the ID was out of range?
	
	setBit(my_short);
	
	fprintf(file, "AP location checked: %lld \n", id);
	
	checkMessages();
	
	return;
};


void doAPInit(){
	//TODO: Can't have these be hard baked... Gotta get them from a config file or from the user or something.
	//AP_Init("archipelago.gg:52885", "EVN", "Dorrulf", "");
	char conn_str[256];
	DWORD result = GetPrivateProfileStringA("AP EVN", "conn_addr", "", conn_str, sizeof(conn_str), ini_path);
	if (result == 0) {
		fprintf(file, "AP CONFIG ERROR: Did not find IP address");
	}
	
	char game_name[64]; //keep username under 64 characters
	result = GetPrivateProfileStringA("AP EVN", "game_name", "EV Nova", game_name, sizeof(game_name), ini_path);
	if (result == 0) {
		fprintf(file, "AP CONFIG ERROR: Did not find valid game name");
	}
	
	char username[64]; //keep username under 64 characters
	result = GetPrivateProfileStringA("AP EVN", "username", "", username, sizeof(username), ini_path);
	if (result == 0) {
		fprintf(file, "AP CONFIG ERROR: Did not find valid username");
	}
	
	char password[64]; //keep username under 64 characters
	result = GetPrivateProfileStringA("AP EVN", "password", "", password, sizeof(password), ini_path);
	if (result == 0) {
		fprintf(file, "AP CONFIG ERROR: Did not find valid password");
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

void saveAndEmit(short truthIndex) {
	truth_cbs[truthIndex] = 1;
	//AP_Send(startingOffset + truthIndex); //If our control bits start at 2000, but the index is at 100, then we need to send ID 2100
	//AP_Send(truthIndex);
	AP_SendItem(truthIndex);	//for some reason, called "SendItem" but expects location ID...
	//NOTE: AP_Send specifies sending the ID of a location, but we just have the control bits... So we'll have to handle that logic server side.
	
	// We have to send LOCATION IDs. Not the bits. But, I don't think we can find the mission ID here all that well, based on the control bit. Unless we intercept another function.
	//	Otherwise, I think we'll have to export a translation file that we can then reference here to do ID lookups (this bit is associated to this mission ID), etc.
	
	//fprintf(file, "saved and sent bit: %d \n", startingOffset + truthIndex);
	fprintf(file, "saved and sent bit: %d \n", truthIndex);
	
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
	
	freopen("stdout.txt", "a", stdout);
	//freopen("stderr.txt", "w", stderr);

    // Attempt to open the file for writing
    file = fopen("output", "a");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
	
	
	// Check for bit / check if we emit
	// CONSIDERATIONS:
	//	We don't have any persistent storage implmented. So what might happen in the following cases:
	//		1. Connection is lost.
	//			Upon restore, will APCpp have the message queue'd and retry to send it to the server, or will we have to detect and handle that?
	//		2. What happens if the game crashes?
	//			APCpp won't have been able to capture the send most likely. Or even if it did, its cache has been cleared...
	for (size_t i = 0; i < mySize; i++) {
		
		//int bitOffset = *dynamicOffset + startingOffset + (short)i;	//adding our offset
		//unsigned char cbit = *(unsigned char *)bitOffset;	//dereference the pointer based on our offset to get the value at that memory byte.
		//unsigned char* bitMem = *dynamicOffset + startingOffset + (short)i;	//adding our offset
		
		/*
		unsigned char* bitMem = *dynamicOffset + (short)i;	//adding our offset
		unsigned char cbit = *bitOffset;	//dereference the pointer based on our offset to get the value at that memory byte.
		*/
		
		int* bitMem = reinterpret_cast<int*>(*dynamicOffset + i);
		unsigned char cbit = *bitMem;
	
		if (cbit > 0) {
			if (truth_cbs[i] < 1) {
				// I think we emit everything, even if it was a local to local effect
				//	1. This keeps the server on the same page
				//	2. The return may be a special handled case, such as credits, that we still want to get but aren't handled by bit logic.
				saveAndEmit(i);
			}
		}
		
		//TODO: Test if changed from 0 to 1 specifically.
		
		// If so, send to our websocket for broadcast to the archipelago server.
		
		//fprintf(file, "%x %d %d\n", bitOffset, startingOffset + (short)i, cbit);
		//fprintf(file, "%x %d\n", (short)i, cbit);
	}
	
	
	
	fprintf(file, "init? %d \n", static_cast<int>(isAPInit));
	
	AP_ConnectionStatus APStatus = AP_GetConnectionStatus();
	
	if (!isAPInit) {
		fprintf(file, "attempt to init AP");
		try {
			doAPInit();
		}
		catch (const std::exception& e) {
			fprintf(file, "exception: %s \n", e.what());
		}
		catch (...) {
			fprintf(file, "unknown error");
		}
		
		APStatus = AP_GetConnectionStatus();
		
		fprintf(file, "init AP: %d \n", APStatus);
		//isAPInit = true;
		isAPInit = AP_IsInit();
	}
	
	fprintf(file, "connection enum: %d \n", APStatus);
	//fprintf(file, "callsMade: %d \n", callsMade);
	
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
