/* videocapture is a tool with no special purpose
 *
 * Copyright (C) 2009 Ronny Brendel <ronnybrendel@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "examplefilter.hpp"
#include <iostream>

using namespace std;


BaseFilter* create()
{
    return static_cast<BaseFilter*>(new ExampleFilter());
}


void destroy(BaseFilter* filter)
{
    delete filter;
}


ExampleFilter::ExampleFilter() : BaseFilter()
{
    cerr << __PRETTY_FUNCTION__ << endl;
}


ExampleFilter::~ExampleFilter()
{
    cerr << __PRETTY_FUNCTION__ << endl;
}

