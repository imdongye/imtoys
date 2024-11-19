/*
    imdongye@naver.com
	fst: 2024-09-08
	lst: 2024-09-08
*/

#ifndef __apps_selector_h_
#define __apps_selector_h_

#include "../application.h"

namespace lim
{
    namespace apps_selector
    {
        extern int wanted_app_idx; // ctrl var
        extern int selected_app_idx; // invo var
        extern int nr_apps;
        extern std::vector<const char*> app_names;
        extern std::vector<const char*> app_dirs;
        extern std::vector<const char*> app_infos;
        extern std::vector<std::function<AppBase*()>> app_constructors;


        template<typename TApp>
        void add()
        {
            nr_apps++;
            app_names.push_back(TApp::APP_NAME);
            app_dirs.push_back(TApp::APP_DIR);
            app_infos.push_back(TApp::APP_INFO);
            app_constructors.push_back( [](){ return new TApp(); } );
        }

        inline void run() 
        {
            while( wanted_app_idx>=0 )
            {
                selected_app_idx = wanted_app_idx;
                wanted_app_idx = -1;

                AppBase::g_app_name = app_names[selected_app_idx];
                AppBase::g_app_dir =  app_dirs[selected_app_idx];
                AppBase::g_app_info = app_infos[selected_app_idx];

                AppBase::g_ptr = app_constructors[selected_app_idx]();

                AppBase::g_ptr->run();

                delete AppBase::g_ptr;
	        }
        }

        void __drawGui(); // using in AppBase
    }
}

#endif