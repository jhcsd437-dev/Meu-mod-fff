#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_corigaff_application_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Mod Carregado com Sucesso!";
    return env->NewStringUTF(hello.c_str());
}
