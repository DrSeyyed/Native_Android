FROM ubuntu:22.04

LABEL DrSeyyed <seyyedhasanlangari@gmail.com>

ARG USER=android

RUN dpkg --add-architecture i386
RUN apt-get update && apt-get install -y \
        build-essential git neovim wget unzip sudo \
        libc6:i386 libncurses5:i386 libstdc++6:i386 lib32z1 libbz2-1.0:i386 \
        libxrender1 libxtst6 libxi6 libfreetype6 libxft2 xz-utils vim \
        qemu qemu-kvm libvirt-daemon-system bridge-utils libnotify4 libglu1 libqt5widgets5 openjdk-17-jdk xvfb \
        && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN groupadd -g 1000 -r $USER
RUN useradd -u 1000 -g 1000 --create-home -r $USER
RUN adduser $USER libvirt
RUN adduser $USER kvm
# Change password
RUN echo "$USER:$USER" | chpasswd
# Make sudo passwordless
RUN echo "${USER} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/90-$USER
RUN usermod -aG sudo $USER
RUN usermod -aG plugdev $USER

RUN echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="04e8", ATTR{idProduct}=="6860", MODE="0660", GROUP="plugdev", SYMLINK+="android%n"' > /etc/udev/rules.d/51-android.rules 

USER $USER

WORKDIR /home/$USER

# Install Flutter
ARG FLUTTER_URL=https://storage.googleapis.com/flutter_infra_release/releases/stable/linux/flutter_linux_3.13.6-stable.tar.xz
ARG FLUTTER_VERSION=3.13.6

RUN wget "$FLUTTER_URL" -O flutter.tar.xz
RUN tar -xvf flutter.tar.xz
RUN rm flutter.tar.xz

# Android Studio
ARG ANDROID_STUDIO_URL=https://redirector.gvt1.com/edgedl/android/studio/ide-zips/2023.1.1.20/android-studio-2023.1.1.20-linux.tar.gz
ARG ANDROID_STUDIO_VERSION=2023.1.1.20

RUN wget "$ANDROID_STUDIO_URL" -O android-studio.tar.gz
RUN tar xzvf android-studio.tar.gz
RUN rm android-studio.tar.gz

ENV ANDROID_EMULATOR_USE_SYSTEM_LIBS=1

ARG NDK_VER=27.0.11718014
ENV ANDROID_SDK_ROOT /home/$USER/Sdk
ENV JAVA_HOME=/home/android/android-studio/jbr  
ENV ANDROID_NDK_HOME=$ANDROID_SDK_ROOT/ndk/$NDK_VER


ENV PATH ${PATH}:${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin:${ANDROID_SDK_ROOT}/cmdline-tools/bin:${ANDROID_SDK_ROOT}/platform-tools/

RUN mkdir -p ${ANDROID_SDK_ROOT}/cmdline-tools \
    && cd ${ANDROID_SDK_ROOT}/cmdline-tools \
    && wget https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip -O commandlinetools.zip && unzip commandlinetools.zip -d ${ANDROID_SDK_ROOT}/cmdline-tools \
    && mv ${ANDROID_SDK_ROOT}/cmdline-tools/cmdline-tools ${ANDROID_SDK_ROOT}/cmdline-tools/latest \
    && rm commandlinetools.zip

RUN yes | sdkmanager --sdk_root=${ANDROID_SDK_ROOT} --licenses || true \
    && sdkmanager --channel=2 "emulator" "sources;android-35" "system-images;android-35;google_apis;x86_64" "platform-tools" "platforms;android-35" "build-tools;35.0.1" "ndk;$NDK_VER"    


# Clone your Android project into the container
RUN sudo git clone https://github.com/DrSeyyed/Native_Android.git /home/$USER/Project
RUN sudo chown -R $USER:$USER /home/$USER/Project

RUN sudo touch /usr/local/bin/docker_entrypoint.sh
RUN sudo chown $USER:$USER /usr/local/bin/docker_entrypoint.sh
RUN echo '#!/bin/bash' >> /usr/local/bin/docker_entrypoint.sh
RUN echo 'echo "`whoami`" | sudo -S chmod 777 /dev/kvm > /dev/null 2>&1' >> /usr/local/bin/docker_entrypoint.sh
RUN echo 'args="~/android-studio/bin/studio.sh"' >> /usr/local/bin/docker_entrypoint.sh
RUN echo 'exec $args' >> /usr/local/bin/docker_entrypoint.sh
RUN chmod +x /usr/local/bin/docker_entrypoint.sh

RUN avdmanager create avd -n Pixel_6_android_35 -k "system-images;android-35;google_apis;x86_64" --device "pixel_6" --force --sdcard 512M --abi x86_64 --tag google_apis

ENTRYPOINT [ "/usr/local/bin/docker_entrypoint.sh" ]
