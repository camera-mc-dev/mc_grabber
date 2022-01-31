#include "imgio/fake.h"

int main(int argc, char* argv[])
{
	FakeGrabber grabber(0);
	cout << grabber.GetNumCameras() << endl;
}