Java JNI Stub
-------------

Goal: Provide `class G16 { static MetaState update(MetaState st, MetaState stm1, Inputs in); }` via JNI calling `g16_update`.

Files to add later:
- `src/main/java/.../G16.java`
- `src/main/native/g16_jni.cpp`
- Gradle or Maven config linking against `libg16_cabi.a`.

