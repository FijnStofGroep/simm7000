/**
 * @file       TinyGsmClient.h
 * @author     Volodymyr Shymanskyy
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy
 * @date       Nov 2016
 */

#ifndef SRC_TINYGSMCLIENT_H_
#define SRC_TINYGSMCLIENT_H_

#if defined(TINY_GSM_MODEM_SIM7000)
#include "TinyGsmClientSIM7000.h"
typedef TinyGsmSim7000 TinyGsm;
typedef TinyGsmSim7000::GsmClientSim7000 TinyGsmClient;

#elif defined(TINY_GSM_MODEM_SIM7000SSL)
#include "TinyGsmClientSIM7000SSL.h"
typedef TinyGsmSim7000SSL TinyGsm;
typedef TinyGsmSim7000SSL::GsmClientSim7000SSL TinyGsmClient;
typedef TinyGsmSim7000SSL::GsmClientSecureSIM7000SSL TinyGsmClientSecure;

#else
#error "Please define GSM modem model"
#endif

#endif  // SRC_TINYGSMCLIENT_H_
