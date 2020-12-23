//
// Created by ubuntu on 23.12.2020.
//

#include "grp.request.h"
#include "handler.h"

Request::Request(CHAR_DATA *subject, Group *group, RQ_TYPE type) {
    if (!subject || !group)
        return;
    _applicant = subject;
    _applicantName = subject->get_pc_name();
    _applicantUID = subject->get_uid();
    _group = group;
    _type = type;
}


