#ifndef DEBUG_H
#define DEBUG_H

#define ASSERT(expression) if(!expression)*((int*)0) = 0;
#define ASSERT_SOFT(expression) if(!expression){printf("Assertion '%s' failed!! => at line:%d in file: '%s'\n", #expression, __LINE__, __FILE__);}
#endif //DEBUG_H
