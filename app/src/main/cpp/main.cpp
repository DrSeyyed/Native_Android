#include <jni.h>
#include "AndroidOut.h"
#include "Renderer.h"
#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>
#include <chrono>



extern "C" {
#include <game-activity/native_app_glue/android_native_app_glue.c>
}


auto last_time = std::chrono::high_resolution_clock::now();


/*!
 * Handles commands sent to this Android application
 * @param pApp the app the commands are coming from
 * @param cmd the command to handle
 */
void handle_cmd(android_app *pApp, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_GAINED_FOCUS:
            aout<<"Gained Focus"<<std::endl;
            break;
        case APP_CMD_INIT_WINDOW:
            aout<<"Init windows"<<std::endl;
            // A new window is created, associate a renderer with it. You may replace this with a
            // "game" class if that suits your needs. Remember to change all instances of userData
            // if you change the class here as a reinterpret_cast is dangerous this in the
            // android_main function and the APP_CMD_TERM_WINDOW handler case.
            if(!pApp->userData)
                pApp->userData = new Renderer(pApp);
            if (pApp->userData) {
                //
                auto *pRenderer = reinterpret_cast<Renderer *>(pApp->userData);
                pRenderer->resumeWork();
                last_time = std::chrono::high_resolution_clock::now();
            }
            break;
        case APP_CMD_LOST_FOCUS:
            aout<<"lost Focus"<<std::endl;
            break;
        case APP_CMD_PAUSE:
            aout<<"pause"<<std::endl;
            break;
        case APP_CMD_RESUME:
            aout<<"resume"<<std::endl;
            break;
        case APP_CMD_START:
            aout<<"start"<<std::endl;
            break;
        case APP_CMD_STOP:
            aout<<"stop"<<std::endl;
            break;
        case APP_CMD_SAVE_STATE:
            aout<<"save state"<<std::endl;
            break;
        case APP_CMD_TERM_WINDOW:
            aout<<"term windows"<<std::endl;
            if (pApp->userData) {
                //
                auto *pRenderer = reinterpret_cast<Renderer *>(pApp->userData);
                pRenderer->pauseWork();
            }
            break;
        case APP_CMD_DESTROY:
            aout<<"destroy"<<std::endl;
            // The window is being destroyed. Use this to clean up your userData to avoid leaking
            // resources.
            //
            // We have to check if userData is assigned just in case this comes in really quickly
            if (pApp->userData) {
                //
                auto *pRenderer = reinterpret_cast<Renderer *>(pApp->userData);
                pApp->userData = nullptr;
                delete pRenderer;
            }
            break;
        default:
            break;
    }
}

/*!
 * Enable the motion events you want to handle; not handled events are
 * passed back to OS for further processing. For this example case,
 * only pointer and joystick devices are enabled.
 *
 * @param motionEvent the newly arrived GameActivityMotionEvent.
 * @return true if the event is from a pointer or joystick device,
 *         false for all other input devices.
 */
bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent) {
    auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
            sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
}

//orientation
void set_requested_screen_orientation(struct android_app * app, int an_orientation){

    JNIEnv * jni;
    app->activity->vm->AttachCurrentThread(&jni, NULL);
    jclass clazz = jni->GetObjectClass(app->activity->javaGameActivity);
    jmethodID methodID = jni->GetMethodID(clazz, "setRequestedOrientation", "(I)V");
    jni->CallVoidMethod(app->activity->javaGameActivity, methodID, an_orientation);
    app->activity->vm->DetachCurrentThread();

}

/*!
 * This the main entry point for a native activity
 */
void android_main(struct android_app *pApp) {
    // Can be removed, useful to ensure your code is running
    aout << "Welcome to android_main" << std::endl;
    // Register an event handler for Android events
    set_requested_screen_orientation(pApp,0);
    pApp->onAppCmd = handle_cmd;

    // Set input event filters (set it to NULL if the app wants to process all inputs).
    // Note that for key inputs, this example uses the default default_key_filter()
    // implemented in android_native_app_glue.c.
    android_app_set_motion_event_filter(pApp, motion_event_filter_func);
    // This sets up a typical game/event loop. It will run until the app is destroyed.
    int events;
    android_poll_source *pSource;
    do {
        // Process all pending events before running game logic.
        if (ALooper_pollOnce(0, nullptr, &events, (void **) &pSource) >= 0) {
            if (pSource) {
                pSource->process(pApp, pSource);
            }
        }
        // Check if any user data is associated. This is assigned in handle_cmd
        if (pApp->userData) {

            // We know that our user data is a Renderer, so reinterpret cast it. If you change your
            // user data remember to change it here
            auto *pRenderer = reinterpret_cast<Renderer *>(pApp->userData);
            //make renderer ready for the first time
            // Process game input
            pRenderer->handleInput();
            // Calculate delta time
            auto now_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> diff = last_time - now_time;
            pRenderer->setDeltaTime(diff.count());
            last_time = now_time;
            // Render a frame
            pRenderer->render();
        }
    } while (!pApp->destroyRequested);
}
