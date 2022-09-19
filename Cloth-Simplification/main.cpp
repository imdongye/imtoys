//
//  2022-07-21 / im dongye
//
//  if you mac you must set
//  Product->schema->edit-schema->run->option->custom-working-dir
//

/* for vsprintf_s */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "lim/limclude.h"

// rid unused variables warnings
int main(int, char**)
{
	lim::SimplifyApp app;
	app.run();

	return 0;
}