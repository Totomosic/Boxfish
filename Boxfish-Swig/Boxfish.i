%module Boxfish

%include "typemaps.i"
%include "std_vector.i"
%include "std_string.i"

%{
#include "Boxfish.h"
using namespace Boxfish;
%}

%include "../Boxfish-Lib/src/Boxfish.h"
%include "../Boxfish-Lib/src/Move.h"
%include "../Boxfish-Lib/src/Position.h"
%include "../Boxfish-Lib/src/PositionUtils.h"
%include "../Boxfish-Lib/src/Format.h"
%include "../Boxfish-Lib/src/Search.h"
%include "../Boxfish-Lib/src/Types.h"

%template(MoveVector) std::vector<Boxfish::Move>;
