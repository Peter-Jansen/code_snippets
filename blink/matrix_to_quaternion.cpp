float4 matrixToQuaternion(float3x3 m) {
    float4 q;
    
    // Calculate the trace of the matrix
    float trace = m[0][0] + m[1][1] + m[2][2];
    
    if (trace > 0.0) {
        float s = sqrt(trace + 1.0) * 2.0;
        q.w = s * 0.25;
        q.x = (m[2][1] - m[1][2]) / s;
        q.y = (m[0][2] - m[2][0]) / s;
        q.z = (m[1][0] - m[0][1]) / s;
    }
    else if ((m[0][0] > m[1][1]) && (m[0][0] > m[2][2])) {
        float s = sqrt(1.0 + m[0][0] - m[1][1] - m[2][2]) * 2.0;
        q.w = (m[2][1] - m[1][2]) / s;
        q.x = s * 0.25;
        q.y = (m[1][0] + m[0][1]) / s;
        q.z = (m[0][2] + m[2][0]) / s;
    }
    else if (m[1][1] > m[2][2]) {
        float s = sqrt(1.0 + m[1][1] - m[0][0] - m[2][2]) * 2.0;
        q.w = (m[0][2] - m[2][0]) / s;
        q.x = (m[1][0] + m[0][1]) / s;
        q.y = s * 0.25;
        q.z = (m[2][1] + m[1][2]) / s;
    }
    else {
        float s = sqrt(1.0 + m[2][2] - m[0][0] - m[1][1]) * 2.0;
        q.w = (m[1][0] - m[0][1]) / s;
        q.x = (m[0][2] + m[2][0]) / s;
        q.y = (m[2][1] + m[1][2]) / s;
        q.z = s * 0.25;
    }
    
    // Normalize the quaternion
    float length = sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    if (length > 0.0) {
        q = q / length;
    }
    
    return q;
} 