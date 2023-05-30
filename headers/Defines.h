//#pragma once
//Deprecation Defines
#define _CRT_SECURE_NO_WARNINGS

//OpenGL Defines
#define OPENGL_SUPRESS_NOTIFICATION

//Tinyglft Defines
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

//Enable MSAA
//#define MSAAOFF
#define MSAA4X
//#define MSAA8X

//Current Project
#define NORMAL_MAPS


//TODO: Load_OBJ_TAN is computing tangents as per face instead of per vertex, figure that out
// so normal maps dont look flat shaded

//Double check, but I can probably remove the entire original rook folder and files

//Get a proper shadow mapping example working

//Remove all unused resource files

//Try to get glb based model loading working again with tangents




//FINAL: do a final deferred shader that includes blinn-phong, shadows, enviroment mapping and normal mapping