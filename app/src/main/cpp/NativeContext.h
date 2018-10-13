//
// Created by gfm13 on 10/13/2018.
//

#ifndef ARCHESS_NATIVECONTEXT_H
#define ARCHESS_NATIVECONTEXT_H
#include <android/asset_manager.h>

class NativeContext {
public:
    NativeContext(AAssetManager* assetManager);
private:
    AAssetManager* assetManager;

};


#endif //ARCHESS_NATIVECONTEXT_H
