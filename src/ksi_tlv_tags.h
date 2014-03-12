#ifndef KSI_TLV_TAGS_H_
#define KSI_TLV_TAGS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define KSI_TLV_TAG_HEADER 0x01
#define KSI_TLV_TAG_HEADER_INSTENCE_ID	0x05
#define KSI_TLV_TAG_HEADER_MESSAGE_ID	0x06
#define KSI_TLV_TAG_HEADER_CLIENT_ID	0x07

#define KSI_TLV_TAG_PDU_AGGREGATION		0x0200
#define KSI_TLV_TAG_AGGR_REQUEST		0x0201
#define KSI_TLV_TAG_AGGR_RESPONSE		0x0202
#define KSI_TLV_TAG_AGGR_REQUEST_ID		0x02
#define KSI_TLV_TAG_AGGR_REQUEST_HASH	0x03
#define KSI_TLV_TAG_AGGR_LENGTH_VALUE	0x04
#define KSI_TLV_TAG_AGGR_ERROR			0x05
#define KSI_TLV_TAG_AGGR_MAC			0x1f

#define KSI_TLV_TAG_AGGR_RESPONSE_REQ_ACK			0x12
#define KSI_TLV_TAG_AGGR_RESPONSE_REQ_ACK_PERIOD	0x01
#define KSI_TLV_TAG_AGGR_RESPONSE_REQ_ACK_DELAY		0x02

#define KSI_TLV_TAG_AGGR_CONF						0x10
#define KSI_TLV_TAG_AGGRE_CONF_GLOBAL_DEPTH			0x01
#define KSI_TLV_TAG_AGGRE_CONT_MAX_DEPTH			0x02
#define KSI_TLV_TAG_AGGRE_CONF_AGGR_ALGO			0x03
#define KSI_TLV_TAG_AGGRE_CONT_AGGR_PERIOD			0x04
#define KSI_TLV_TAG_AGGRE_CONF_PARENT_SERVER		0x05

#define KSI_TLV_SIGNATURE	0x0800

#ifdef __cplusplus
}
#endif

#endif /* KSI_TLV_TAGS_H_ */
