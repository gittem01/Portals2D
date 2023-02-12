#pragma once

class Portal;

typedef enum
{
    TEST_PLAYER = 0,
    EDGE = 1,
} g_DataType;

typedef struct
{
    Portal* portals[2];
    int count;
} g_EdgeData;

typedef struct
{
    g_DataType type;
    void* data;
} g_PhysicsData;