# Use the official OpenJDK 8 image from Docker Hub
FROM openjdk:17-jdk-buster

# Install necessary packages
RUN apt-get update && apt-get install -y \
    wget \
    unzip \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set environment variables for Android SDK
ENV ANDROID_SDK_ROOT /opt/android-sdk
ENV PATH ${PATH}:${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin:${ANDROID_SDK_ROOT}/platform-tools:${ANDROID_SDK_ROOT}/cmdline-tools/bin

# Download and install Android SDK command-line tools
RUN mkdir -p ${ANDROID_SDK_ROOT}/cmdline-tools \
    && cd ${ANDROID_SDK_ROOT}/cmdline-tools \
    && wget https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip -O commandlinetools.zip \
    && unzip commandlinetools.zip -d ${ANDROID_SDK_ROOT}/cmdline-tools \
    && mv ${ANDROID_SDK_ROOT}/cmdline-tools/cmdline-tools ${ANDROID_SDK_ROOT}/cmdline-tools/latest \
    && rm commandlinetools.zip

# Install SDK, NDK, and Build Tools
RUN yes | sdkmanager --sdk_root=${ANDROID_SDK_ROOT} --licenses || true \
    && sdkmanager "platform-tools" "platforms;android-34" "build-tools;34.0.0" "ndk;27.0.11718014"

# Clone your Android project into the container
RUN git clone https://github.com/DrSeyyed/Native_Android.git /usr/src/app

# Set working directory
WORKDIR /usr/src/app

# Make the gradlew script executable
RUN chmod +x ./gradlew

# Pre-install dependencies to speed up build
RUN ./gradlew dependencies --no-daemon

# Default command to run when starting the container
CMD ["./gradlew", "assembleDebug"]
