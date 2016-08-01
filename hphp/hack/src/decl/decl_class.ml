(**
 * Copyright (c) 2016, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the "hack" directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 *)

open Typing_defs
open Decl_defs
module Inst = Decl_instantiate

let rec apply_substs substs class_context ty =
  match SMap.get class_context substs with
  | None -> ty
  | Some { sc_subst = subst; sc_class_context = next_class_context; _ } ->
    apply_substs substs next_class_context (Inst.instantiate subst ty)

let to_class_type {
  dc_need_init;
  dc_members_fully_known;
  dc_abstract;
  dc_final;
  dc_deferred_init_members;
  dc_kind;
  dc_name;
  dc_pos;
  dc_tparams;
  dc_substs;
  dc_consts;
  dc_typeconsts;
  dc_props;
  dc_sprops;
  dc_methods;
  dc_smethods;
  dc_construct;
  dc_ancestors;
  dc_req_ancestors;
  dc_req_ancestors_extends;
  dc_extends;
  dc_enum_type;
} =
  let apply_subst_to_ce ce =
    let ty = apply_substs dc_substs ce.ce_origin ce.ce_type in
    { ce with ce_type = ty }
  in
  let tc_props = SMap.map apply_subst_to_ce dc_props in
  let tc_sprops = SMap.map apply_subst_to_ce dc_sprops in
  let tc_methods = SMap.map apply_subst_to_ce dc_methods in
  let tc_smethods = SMap.map apply_subst_to_ce dc_smethods in
  let tc_construct = match dc_construct with
    | Some ce, cst -> Some (apply_subst_to_ce ce), cst
    | _ -> dc_construct
  in
  {
    tc_need_init = dc_need_init;
    tc_members_fully_known = dc_members_fully_known;
    tc_abstract = dc_abstract;
    tc_final = dc_final;
    tc_deferred_init_members = dc_deferred_init_members;
    tc_kind = dc_kind;
    tc_name = dc_name;
    tc_pos = dc_pos;
    tc_tparams = dc_tparams;
    tc_consts = dc_consts;
    tc_typeconsts = dc_typeconsts;
    tc_props;
    tc_sprops;
    tc_methods;
    tc_smethods;
    tc_construct;
    tc_ancestors = dc_ancestors;
    tc_req_ancestors = dc_req_ancestors;
    tc_req_ancestors_extends = dc_req_ancestors_extends;
    tc_extends = dc_extends;
    tc_enum_type = dc_enum_type;
  }