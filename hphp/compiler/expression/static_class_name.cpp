/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-present Facebook, Inc. (http://www.facebook.com)  |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#include "hphp/compiler/expression/static_class_name.h"
#include "hphp/compiler/expression/scalar_expression.h"
#include "hphp/compiler/expression/simple_variable.h"
#include "hphp/compiler/statement/statement_list.h"
#include "hphp/compiler/analysis/class_scope.h"
#include "hphp/compiler/analysis/file_scope.h"
#include "hphp/compiler/analysis/variable_table.h"
#include "hphp/util/text-util.h"

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////

StaticClassName::StaticClassName(ExpressionPtr classExp)
    : m_class(classExp),
      m_self(false), m_parent(false), m_static(false),
      m_redeclared(false), m_unknown(true) {
  updateClassName();
  auto const isame = [](const std::string& a, const std::string& b) {
    return (a.size() == b.size()) &&
           !strncasecmp(a.c_str(), b.c_str(), a.size());
  };
  if (isame(m_origClassName, "parent")) {
    m_parent = true;
  } else if (isame(m_origClassName, "self")) {
    m_self = true;
  } else if (isame(m_origClassName, "static")) {
    m_static = true;
    m_class = classExp;
    m_origClassName = "";
  }
}

void StaticClassName::onParse(AnalysisResultConstRawPtr ar,
                              FileScopePtr /*scope*/) {
  if (!m_self && !m_parent && !m_static && hasStaticClass()) {
    ar->parseOnDemandByClass(m_origClassName);
  }
}

bool StaticClassName::isNamed(folly::StringPiece clsName) const {
  return bstrcasecmp(m_origClassName, clsName) == 0;
}

void StaticClassName::updateClassName() {
  if (m_class && m_class->is(Expression::KindOfScalarExpression) &&
      !m_static) {
    auto s = dynamic_pointer_cast<ScalarExpression>(m_class);
    auto const& className = s->getString();
    m_origClassName = className;
    m_class.reset();
  } else {
    m_origClassName = "";
  }
}

ClassScopePtr StaticClassName::resolveClass() {
  m_unknown = true;
  if (m_class) return ClassScopePtr();
  auto scope = dynamic_cast<Construct*>(this)->getScope();
  if (m_self) {
    if (ClassScopePtr self = scope->getContainingClass()) {
      m_origClassName = self->getOriginalName();
      m_unknown = false;
      return self;
    }
  } else if (m_parent) {
    if (ClassScopePtr self = scope->getContainingClass()) {
      if (!self->getOriginalParent().empty()) {
        m_origClassName = self->getOriginalParent();
      }
    } else {
      m_parent = false;
    }
  }
  ClassScopePtr cls = scope->getContainingProgram()->findClass(m_origClassName);
  if (cls) {
    m_unknown = false;
    if (cls->isVolatile()) {
      ClassScopeRawPtr c = scope->getContainingClass();
      if (c && c->isNamed(m_origClassName)) {
        c.reset();
      }
      if (cls->isRedeclaring()) {
        cls = c;
        m_redeclared = true;
      }
    }
  }
  return cls;
}

///////////////////////////////////////////////////////////////////////////////

void StaticClassName::outputPHP(CodeGenerator &cg, AnalysisResultPtr ar) {
  if (m_class) {
    m_class->outputPHP(cg, ar);
  } else {
    cg_printf("%s", m_origClassName.c_str());
  }
}

///////////////////////////////////////////////////////////////////////////////
}
