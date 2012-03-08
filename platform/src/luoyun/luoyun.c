/*
** Copyright (C) 2012 LuoYun Co. 
**
**           Authors:
**                    lijian.gnu@gmail.com 
**                    zengdongwu@hotmail.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**  
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**  
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**  
*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "luoyun.h"
#include "../util/logging.h"

void luoyun_node_ctrl_instance_print(NodeCtrlInstance * ci)
{
    logsimple("Node Ctrl Instance struct:\n"
              "req_id = %d\n"
              "req_action = %d\n"
              "ins_id = %d\n"
              "ins_status = %d\n"
              "ins_name = %s\n"
              "ins_vcpu = %d\n"
              "ins_mem = %d\n"
              "ins_mac = %s\n"
              "ins_ip = %s\n"
              "app_id = %d\n"
              "app_name = %s\n"
              "app_uri = %s\n"
              "app_checksum = %s\n"
              "storage_ip = %s\n"
              "storage_method = %d\n"
              "storage_parm = %s\n"
              "osm_clcip = %s\n"
              "osm_clcport = %d\n"
              "osm_tag = %d\n"
              "osm_secret = %s\n",
              ci->req_id, ci->req_action,
              ci->ins_id, ci->ins_status, ci->ins_name, ci->ins_vcpu,
              ci->ins_mem, ci->ins_mac, ci->ins_ip,
              ci->app_id, ci->app_name, ci->app_uri, ci->app_checksum, 
              ci->storage_ip, ci->storage_method, ci->storage_parm,
              ci->osm_clcip, ci->osm_clcport, ci->osm_tag, ci->osm_secret);
    if (ci->ins_ip == NULL)
        logsimple("instance ip is NULL\n");
}

void luoyun_node_ctrl_instance_cleanup(NodeCtrlInstance * ci)
{
    if (ci == NULL)
        return;
    if (ci->ins_name)
        free(ci->ins_name);
    if (ci->ins_mac)
        free(ci->ins_mac);
    if (ci->ins_ip)
        free(ci->ins_ip);
    if (ci->app_name)
        free(ci->app_name);
    if (ci->app_uri)
        free(ci->app_uri);
    if (ci->app_checksum)
        free(ci->app_checksum);
    if (ci->storage_ip)
        free(ci->storage_ip);
    if (ci->storage_parm)
        free(ci->storage_parm);
    if (ci->osm_clcip)
        free(ci->osm_clcip);
    if (ci->osm_secret)
        free(ci->osm_secret);
    bzero(ci, sizeof(NodeCtrlInstance));
    return;
}

NodeCtrlInstance * luoyun_node_ctrl_instance_copy(NodeCtrlInstance * ci)
{
    if (ci == NULL)
        return NULL;

    NodeCtrlInstance *ret = malloc(sizeof(NodeCtrlInstance));
    if (ret == NULL)
        return NULL;
    *ret = *ci;
    if (ci->ins_name)
        ret->ins_name = strdup(ci->ins_name);
    if (ci->ins_mac)
        ret->ins_mac = strdup(ci->ins_mac);
    if (ci->ins_ip)
        ret->ins_ip = strdup(ci->ins_ip);
    if (ci->app_name)
        ret->app_name = strdup(ci->app_name);
    if (ci->app_uri)
        ret->app_uri = strdup(ci->app_uri);
    if (ci->app_checksum)
        ret->app_checksum = strdup(ci->app_checksum);
    if (ci->osm_clcip)
        ret->osm_clcip = strdup(ci->osm_clcip);
    if (ci->storage_ip)
        ret->storage_ip = strdup(ci->storage_ip);
    if (ci->storage_parm)
        ret->storage_parm = strdup(ci->storage_parm);
    if (ci->osm_secret)
        ret->osm_secret = strdup(ci->osm_secret);
    return ret;
}

void luoyun_node_info_print(NodeInfo * nf)
{
    logsimple("Node Info = {\n"
              "\tstatus = %d\n"
              "\thostname = %s\n"
              "\tip = %s\n"
              "\tport = %d\n"
              "\tarch = %d\n"
              "\thypervisor = %d\n"
              "\tnetwork_type = %d\n"
              "\tmax_memory = %d\n"
              "\tmax_cpus = %d\n"
              "\tcpu_model = %s\n"
              "\tcpu_mhz = %d\n"
              "\tload_average = %d\n" "\tfree_memory = %d\n"
              "}\n",
              nf->status, nf->hostname, nf->ip, nf->port,
              nf->arch, nf->hypervisor, nf->network_type,
              nf->max_memory, nf->max_cpus, nf->cpu_model,
              nf->cpu_mhz, nf->load_average, nf->free_memory);
}

void luoyun_node_info_cleanup(NodeInfo * nf)
{
    if (nf == NULL)
        return;
    if (nf->cpu_model)
        free(nf->cpu_model);
    if (nf->hostname)
        free(nf->hostname);
    if (nf->ip)
        free(nf->ip);
    bzero(nf, sizeof(NodeInfo));
}

void luoyun_instance_info_print(InstanceInfo *ii)
{
    logsimple("Instance Info struct:\n"
              "ip = %s\n"
              "status = %d\n",
              ii->ip, ii->status);
}

void luoyun_instance_info_cleanup(InstanceInfo *ii)
{
    if (ii == NULL)
        return;
    if (ii->ip)
        free(ii->ip);
    bzero(ii, sizeof(InstanceInfo));
}

void luoyun_osm_info_print(OSMInfo *oi)
{
    logsimple("Instance Info struct:\n"
              "ip = %s\n"
              "tag = %d\n"
              "status = %d\n",
              oi->ip, oi->tag, oi->status);
}

void luoyun_osm_info_cleanup(OSMInfo *oi)
{
    if (oi == NULL)
        return;
    if (oi->ip)
        free(oi->ip);
    bzero(oi, sizeof(OSMInfo));
    oi->tag = -1;
}

