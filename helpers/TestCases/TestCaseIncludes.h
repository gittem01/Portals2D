#include <TestCase1.h>
#include <TestCase2.h>
#include <TestCase3.h>
#include <TestCase4.h>
#include <TestCase5.h>
#include <TestCase6.h>
#include <TestCase7.h>
#include <TestCase8.h>

BaseTestCase* newTest(WindowPainter* wh, int testCaseNum){
    switch (testCaseNum)
    {
        case 1:
            return new TestCase1(wh);
        case 2:
            return new TestCase2(wh);
        case 3:
            return new TestCase3(wh);
        case 4:
            return new TestCase4(wh);
        case 5:
            return new TestCase5(wh);
        case 6:
            return new TestCase6(wh);
        case 7:
            return new TestCase7(wh);
        case 8:
            return new TestCase8(wh);
        default:
            return nullptr;
    }
}