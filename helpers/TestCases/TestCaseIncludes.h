#include <TestCase1.h>
#include <TestCase2.h>
#include <TestCase3.h>
#include <TestCase4.h>
#include <TestCase5.h>
#include <TestCase6.h>
#include <TestCase7.h>
#include <TestCase8.h>

BaseTestCase* newTest(Game* game, int testCaseNum)
{
    switch (testCaseNum)
    {
        case 1:
            return new TestCase1(game);
        case 2:
            return new TestCase2(game);
        case 3:
            return new TestCase3(game);
        case 4:
            return new TestCase4(game);
        case 5:
            return new TestCase5(game);
        case 6:
            return new TestCase6(game);
        case 7:
            return new TestCase7(game);
        case 8:
            return new TestCase8(game);
        default:
            return nullptr;
    }
}