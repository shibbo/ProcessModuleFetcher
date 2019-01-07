#include <string.h>
#include <stdio.h>

#include <switch.h>

const u32 MAX_PROCESS_SIZE = 0x40;
const u32 MAX_MODULE_SIZE = 0x10;

int main(int argc, char **argv)
{
    // result storage
    Result res = 0;
    // process and title id storage
    u32 numProcesses = 0;
    u64 processIDs[0x40];
    u64 curTitleID = 0;
    u32 curProcessIdx = 0;
    // module information
    bool needsGetModule;
    u32 numModules;
	// permission related
	MemoryInfo memInfo;
	u32 pageInfo;
    // drawing
    bool needsUpdate = true;
    
    // initalize the console
    consoleInit(NULL);
    
    // initialize this
    res = pminfoInitialize();
    
    if (R_FAILED(res))
        printf("pminfoInitialize() failed. 0x%x\n", res);
    
    // initialize the loader
    res = ldrDmntInitialize();
    
    if (R_FAILED(res))
        printf("ldrDmntInitialize() failed. 0x%x\n", res);
    
    // get our process list
    res = svcGetProcessList(&numProcesses, processIDs, MAX_PROCESS_SIZE); 
    
    if (R_FAILED(res))
        printf("svcGetProcessList() failed. 0x%x\n", res);
    else
        printf("Got %d processes.\n", numProcesses);
    
    // print basic useage
    printf("Press the PLUS key to exit.\n");
    printf("Use the UP and DOWN buttons on the joycons to scroll through the list of process IDS.\n");
    printf("Use the X key to view the information stored in modules inside the application.\n");
    
     while(appletMainLoop())
     {
        hidScanInput();
        
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        
        /*
            The controls are as follows:
            UP brings the list upwards to the next process
            DOWN brings the list downwards to the previous process
            X brings up the module information for the title.
            PLUS exits the application.
        */
        if (kDown & KEY_PLUS) 
            break;
        
        if (kDown & KEY_UP)
        {
            // if our current index is the top process, we loop back to the beginning
            if (curProcessIdx == MAX_PROCESS_SIZE - 1)
                curProcessIdx = 0;
            else
                curProcessIdx++;
            
            needsUpdate = true; 
        }
        
        if (kDown & KEY_DOWN)
        {
            // if our current index is the bottom process, we loop back up to the end
            if (curProcessIdx == 0)
                curProcessIdx = MAX_PROCESS_SIZE - 1;
            else
                curProcessIdx--;
            
            needsUpdate = true;
        }
        
        // tell the console to write our module information
        if (kDown & KEY_X)
        {
            needsGetModule = true;
            needsUpdate = true;
        }
        
        if (needsUpdate)
        {
            // first we get the title id of the process with a process id
            res = pminfoGetTitleId(&curTitleID, processIDs[curProcessIdx]);
            
            if (R_FAILED(res))
            {
                printf("Failed to get Title ID from process. 0x%x\n", res);
                needsUpdate = false;
                continue;
            }
            
            // print it out
            printf("Process ID [#%d]: %lu (Title ID: %lx)\n", curProcessIdx, processIDs[curProcessIdx], curTitleID);
            
            if (needsGetModule)
            {
                // module counts can be anything, so I just define a set size (which should be good for most games...)
                LoaderModuleInfo moduleInfos[MAX_MODULE_SIZE];
                // and we get them
                res = ldrDmntGetModuleInfos(processIDs[curProcessIdx], moduleInfos, sizeof(moduleInfos), &numModules);
                
                if (R_FAILED(res))
                {
                    printf("ldrDmntGetModuleInfos() failed. 0x%x\n", res);
                    needsUpdate = false;
                    continue;
                }
                
                // I have a doubt this will actually be a problem
                if (numModules == 0)
                {
                    printf("Number of modules is 0!\n");
                    needsUpdate = false;
                    continue;
                }
                
                // a quick message for me to up the max count
                if (numModules > MAX_MODULE_SIZE)
                    printf("The number of modules exceeds the max defined number of modules.\n");
                
                printf("Number of modules: %d\n\n", numModules);
                
                // now we iterate through each module (we already know the true count)
                for (int i = 0; i < numModules; i++)
                {
                    printf("Module Number: %d\n", i);
                    printf("Base Address: 0x%lx\n", moduleInfos[i].base_address);
                    printf("Module Size: 0x%lx\n\n", moduleInfos[i].size);
					
					res = svcQueryMemory(&memInfo, &pageInfo, moduleInfos[i].base_address);
					
					if (R_FAILED(res))
					{
						printf("svcQueryMemory() failed. 0x%x\n", res);
						needsUpdate = false;
						continue;
					}
					
					printf("Permissions:\n");
					printf("Full Value: %d\n", memInfo.perm);
					printf("R: %d\n", (memInfo.perm) & 0x01);
					printf("W: %d\n", (memInfo.perm >> 1) & 0x01);
					printf("X: %d\n\n", (memInfo.perm >> 2) & 0x01);
					
                }
            }
            
            needsUpdate = false;
        }

        consoleUpdate(NULL);
    }
     
    // exit our services
    pminfoExit();
    ldrDmntExit();
    
    consoleExit(NULL);
    
    return 0;
}
