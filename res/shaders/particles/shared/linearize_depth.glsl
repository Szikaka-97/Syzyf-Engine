float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * Global_CameraNearPlane * Global_CameraFarPlane) / (Global_CameraFarPlane + Global_CameraNearPlane - z * (Global_CameraFarPlane - Global_CameraNearPlane));
}
