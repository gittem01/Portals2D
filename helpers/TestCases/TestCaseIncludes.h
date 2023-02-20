#include <TestCase1.h>
#include <TestCase2.h>
#include <TestCase3.h>
#include <TestCase4.h>
#include <TestCase5.h>
#include <TestCase6.h>
#include <TestCase7.h>
#include <TestCase8.h>

BaseTestCase* newTest(ThinDrawer* td, int testCaseNum)
{
    switch (testCaseNum)
    {
        case 1:
            return new TestCase1(td);
        case 2:
            return new TestCase2(td);
        case 3:
            return new TestCase3(td);
        case 4:
            return new TestCase4(td);
        case 5:
            return new TestCase5(td);
        case 6:
            return new TestCase6(td);
        case 7:
            return new TestCase7(td);
        case 8:
            return new TestCase8(td);
        default:
            return nullptr;
    }
}