#include<iostream>
#include<Windows.h>
#include<d3d9.h>
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")
#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")
#include"hookTemplate.h"
#include"hackLogic.h"

using aliasEndscene = HRESULT( __stdcall* )(IDirect3DDevice9*);
aliasEndscene  EndScenePtr { nullptr };

// using hooking class here
HooknPatch hNP {};
// Pointer to our HackLOgic class containing all offsets and 
// function( like init,update,checkValidEnt,and worldToScreen)
HackLogic* hackLogic; 
int gameWindowWidth {};
int gameWindowHeight {};

// Get the size of Gamewindow  
void getWindowSize( HWND gamewindow)
{
	RECT rect {};
	if (GetWindowRect( gamewindow, &rect ))
	{
		gameWindowWidth = rect.right - rect.left;
		gameWindowHeight = rect.bottom - rect.top;

	}
}
// Drawing lines here by Calling ID3DXLine interface 
 // and D3DXCreateLine function to get address of interface
void DrawLine( IDirect3DDevice9* pDevice,float x1, float y1, float x2, float y2, float width,bool antialias,D3DCOLOR color )
{
	ID3DXLine* pLine {}; // Interface implements line drawing using textured triangles
	D3DXCreateLine( pDevice, &pLine );
	D3DXVECTOR2 line[] = { D3DXVECTOR2( x1,y1 ),D3DXVECTOR2( x2,y2 ) };
	pLine->SetWidth( width );
	if (antialias)
		pLine->SetAntialias( 1 );
	pLine->Begin();
	pLine->Draw( line, 2, color );
	pLine->End();
	pLine->Release();
}




//drawing filled rectangle function to be called in hookedEndscene
void DrawFillRect( IDirect3DDevice9* pDevice, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b )
{
	D3DCOLOR rectColor = D3DCOLOR_XRGB( r, g, b );	//No point in using alpha because clear & alpha dont work!
	D3DRECT BarRect = { x, y, x + w, y + h };

	pDevice->Clear( 1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_TARGET, rectColor, 0, 0 );
}

// Create our hookFunction having prototype as Endscene
HRESULT  __stdcall hookEndScene( IDirect3DDevice9* pDevice )
{
	//std::cout << " I got hooked. now you can draw.\n";
	// Do our drawing stuff here
	// 
	//random rectangle
	//DrawFillRect( pDevice, 25, 25, 100, 100,  255, 255, 255 ) ;

	// crosshair
	  //DrawFillRect( pDevice,(gameWindowWidth / 2 - 2), (gameWindowHeight / 2 - 2), 4, 4,255, 255, 255 );
	  // looping for entitties
	  LocalEntity* nowEntPtr {}; // entity pointer check for all other entities
	  for (int i = 1; i < 32; ++i)
	  {
	nowEntPtr = hackLogic->entList->entlistObjPtrArray[i].entityObjPtr;// Entity pointer should not be redefined

//If the current entity does not pass validity condition of hackLogic check
//(health,dormant,nullptr) and being self we skip to the for loop end for next entity
		  if (!(hackLogic->checkValidEnt( nowEntPtr )))
			  continue; // Skips to for Loop End for next entity

			  D3DCOLOR color {};
			  //Color variable for Team and Enemy after check of entity
			  if (nowEntPtr->iTeamNum == hackLogic->localEntity->iTeamNum)
				  color = D3DCOLOR_ARGB( 255, 0, 255, 0 );
			  else
				  color = D3DCOLOR_ARGB( 255, 255, 0, 0 );

			  Vec2 entPosition2D {}; // screen cordinates of entity
			  //This is check for worldToscreen presence for each entity(true or false)
			  if (hackLogic->worldToScreen( nowEntPtr->vecOrigin, entPosition2D ))
				  //Line drawing goes here if entity is having a scren coordinate
				 DrawLine( pDevice, entPosition2D.x, entPosition2D.y, gameWindowWidth / 2, gameWindowHeight, 2, false, color );  
		  
		  //return back to original function using EndScenePtr function pointer	  
	  }
	  return EndScenePtr( pDevice );
}


void* FindEndScene( HWND gameWindow )
{
	constexpr int  endscene_index = 42;// index of Endscene function in VTable. Same for all D3D9.dll
	constexpr int present_index = 17; // index of Present function in Vtable. same for all D3D9.dll
	//function to create a pointer to IDirect3D9 interface
	//methods of IDirect3D9 interface is used to create MS Direct3D objects and set up environment
	// This interace includes methods for enumerating and retrieving cpabilities of device
	//IDirect3D9 interface allows creation of IDirect3DDevice9 objects
	IDirect3D9* pD3D = NULL;
	if (NULL == (pD3D = Direct3DCreate9( D3D_SDK_VERSION )))
	{
		std::cout << " IDirect3D9 interface pointer not returned.\n";
		return nullptr;
	}
	else
	{
		std::cout << " IDirect3D9* pObject--->\t" << std::hex << pD3D << '\n';
	}
	
	if (pD3D)

	{
		// Now initialize value for D3DPRESENT_PARAMETERS structure that is used to create Direct3D device
		D3DPRESENT_PARAMETERS d3dpp {};
		d3dpp.BackBufferFormat = D3DFMT_R5G6B5;
		d3dpp.BackBufferCount = 1;
		d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.hDeviceWindow = gameWindow;
		d3dpp.Windowed = TRUE;
		d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

		//now use IDirect3D9* interface with createdevice() function 
		// This will return address of IDirect3DDevice9* interface in form of pDevice pointer
		//Various implementation methods can be used from IDirect3DDevice9* interface
		//returns fully working device interface,set to the rqd display mode 
		//and allocated with appropriate back-buffers

		IDirect3DDevice9* pDevice {}; // interface pointer to IDirect3Ddevice9
		if (SUCCEEDED( pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, gameWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &pDevice ) ))
		{
			
			// These two lines get the address of vTable functions present in IDirect3DDevice9 interface
			// first read value of Vtable from class pointer pDevice(interface class pointer)
			//Then add the desired index to reah the desired function pointer
			// Then read the value of function address from that found in step(3)
			//cast it to void*

			void* presentAddress = (void*) *(std::intptr_t*) (*(std::intptr_t*) pDevice + (present_index * sizeof( std::intptr_t )));

			void* endSceneAddress = (void*) *(std::intptr_t*) (*(std::intptr_t*) pDevice + (endscene_index * sizeof( std::intptr_t )));

			std::cout << " IDirect3DDevice9* pDevice --->\t" << std::hex << pDevice << '\n';
			std::cout << " Present() addrs --->\t" << std::hex << presentAddress << '\n';
			std::cout << " EndScene() addrs --->\t" << std::hex << endSceneAddress << '\n';

			//pDevice->Release(); // release the IDirect3DDevice9* interface pDevice created to prevent memory leaks
			return endSceneAddress;
		}
		
		//pD3D->Release(); // release the IDirect3D9* interface object pD3D created to prevent memory leaks
	}
	return nullptr;
}

DWORD WINAPI MyThreadFunction( HMODULE hinstDLL )
{
	//Create a console 
	AllocConsole();
	FILE* f;
	freopen_s(&f,"CONOUT$","w",stdout);

		// Find the  game window
	HWND game_window = FindWindowA( NULL, "Counter-Strike: Global Offensive - Direct3D 9" );
	if (!game_window)
	{
		std::cout << " Game-Window HWND find failed. Error: " << GetLastError() << '\n';
	}
	else
	{
		// If Game-window is found we will Find the Endscene address
		getWindowSize( game_window);

		// If Game-window is found we will Find the Endscene address
		EndScenePtr = (aliasEndscene) FindEndScene( game_window );
	}
	// and Type-cast it to Endscene function pointer type-alias to EndScenePtr variable
		// This will assign the Real EndScene address to EndScene Pointer
	// Now we will use the EndScenePtr holding value of real endscene function to create a Trampoline hook to our function 
	
	uintptr_t lpOriginalAddress = (uintptr_t) EndScenePtr; // src original function address saved here

	//Hook endscene with our templated hook class
	// 7 is the length of stolen bytes
	EndScenePtr = (aliasEndscene) hNP.trampHook<7>( (char*) lpOriginalAddress, (char*) &hookEndScene );
	
	// new class dynamically created on heap for dRawing Lines logic
	hackLogic = new HackLogic {}; 

	hackLogic->Init();

	while (!GetAsyncKeyState( VK_END ) & 1)
	{
	// Update function from HackLogic called here
// This update function continuously updates the ViewMatrix float values as player camera 
		hackLogic->Update();
		//Sleep( 10 );
	}

	//UnHook and unload dll
	hNP.patchByte<7>( (char*)lpOriginalAddress );
	FreeLibraryAndExitThread( hinstDLL, 0 );
	CloseHandle( hinstDLL );
	return 0;
}

BOOL WINAPI DllMain( HMODULE hinst__DLL, DWORD fdwReason, LPVOID lpvReserved )
{

	//DWORD ModuleBase = (DWORD)hinst__DLL;
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH: // A process is loading the DLL.
		
		::DisableThreadLibraryCalls( hinst__DLL ); // disable unwanted thread notifications to reduce overhead
		
		CreateThread( nullptr, 0, (LPTHREAD_START_ROUTINE) MyThreadFunction, hinst__DLL, 0, nullptr ); //init our hooks
		break;

		case DLL_PROCESS_DETACH: // A process unloads the DLL.
		
		break;
	}
	return TRUE;
}