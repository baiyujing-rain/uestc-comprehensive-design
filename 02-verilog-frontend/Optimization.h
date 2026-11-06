#pragma once

#ifndef OPTIMIZATION_H
#define OPTIMIZATION_H

#include "MyParser.h"

#include <string>

std::string ExprKey(const ExprNode *expr);
void OptimizeModuleAst(ModuleNode *module);

#endif
