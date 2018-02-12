
#include <Windows.h>
#include <locale>

struct ProcessPipe
{
	HANDLE g_hChild_Rd = NULL;
	HANDLE g_hChild_Wr = NULL;
};
/* Create a pipe allowing reading from child process stdout
*/
ProcessPipe createReadPipe()
{
	ProcessPipe pipe;
	SECURITY_ATTRIBUTES saAttr;
	// Set the bInheritHandle flag so pipe handles are inherited. 
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT. 
	if (!CreatePipe(&pipe.g_hChild_Rd, &pipe.g_hChild_Wr, &saAttr, 0))
		throw std::runtime_error("StdoutRd CreatePipe");

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(pipe.g_hChild_Rd, HANDLE_FLAG_INHERIT, 0))
		throw std::runtime_error("Stdout SetHandleInformation");
	return pipe;
}

/* Create a pipe allowing writing to child process stdout
*/
ProcessPipe createWritePipe()
{
	ProcessPipe pipe;
	SECURITY_ATTRIBUTES saAttr;
	// Set the bInheritHandle flag so pipe handles are inherited. 
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT. 
	if (!CreatePipe(&pipe.g_hChild_Rd, &pipe.g_hChild_Wr, &saAttr, 0))
		throw std::runtime_error("Stdin CreatePipe");

	// Ensure the write handle to the pipe for STDIN is not inherited. 
	if (!SetHandleInformation(pipe.g_hChild_Wr, HANDLE_FLAG_INHERIT, 0))
		throw std::runtime_error("Stdin SetHandleInformation");
	return pipe;
}
void ReadFromPipe(ProcessPipe &pipe)

// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT. 
// Stop when there is no more data. 
{
	const int BUFSIZE = 4096;
	DWORD dwRead, dwWritten;
	CHAR chBuf[BUFSIZE];
	BOOL bSuccess = FALSE;
	HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	COMMTIMEOUTS timeouts = { 0, //interval timeout. 0 = not used
		0, // read multiplier
		10, // read constant (milliseconds)
		0, // Write multiplier
		0  // Write Constant
	};
	SetCommTimeouts(pipe.g_hChild_Rd, &timeouts);

	for (;;)
	{
		bSuccess = ReadFile(pipe.g_hChild_Rd, chBuf, BUFSIZE, &dwRead, 0);
		if (!bSuccess || dwRead == 0) break;

		bSuccess = WriteFile(hParentStdOut, chBuf,
			dwRead, &dwWritten, NULL);
		if (!bSuccess) break;
	}
}
