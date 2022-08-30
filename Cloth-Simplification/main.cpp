//
//  2022-07-21 / im dongye
//
//  if you mac you must set
//  Product->schema->edit-schema->run->option->custom-working-dir
//

#include "lim/app_simplify.h"


// rid unused variables warnings
int main(int, char**)
{
	lim::SimplifyApp app;
	app.run();

	return 0;
}