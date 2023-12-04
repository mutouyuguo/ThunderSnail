#include "requests.h"
#include "dpu.h"

#ifndef DPU_BINARY
#define DPU_BINARY "dpu_task"
#endif

int cmpfunc (const void * a, const void * b)
{
   return ( *(int*)b - *(int*)a );
}

static void ReadDpuSetLog(struct dpu_set_t set) {
    struct dpu_set_t dpu;
    DPU_FOREACH(set, dpu)
    {
        DPU_ASSERT(dpu_log_read(dpu, stdout));
    }
}

void SendSetDpuIdReq() {
  // prepare dpu buffers
  uint8_t *buffers[NUM_DPU];
  size_t sizes[NUM_DPU];
  BufferBuilder builders[NUM_DPU];
  CpuToDpuBufferDescriptor bufferDescs[NUM_DPU];
  for (int i = 0; i < NUM_DPU; i++) {
    bufferDescs[i] = (CpuToDpuBufferDescriptor) {
      .header = {
	.epochNumber = GetEpochNumber(),
      }
    };
  }
  for (int i = 0; i < NUM_DPU; i++) {
    BufferBuilderInit(&builders[i], &bufferDescs[i]);
    BufferBuilderBeginBlock(&builders[i], SET_DPU_ID_REQ);
  }
  for (int i = 0; i < NUM_DPU; i++){
    int dpuIdx = i;
    SetDpuIdReq req;
    req.base = (Task) { .taskType = SET_DPU_ID_REQ };
    req.dpuId = (uint32_t) dpuIdx;
    // append one task for each dpu
    BufferBuilderAppendTask(&builders[dpuIdx], (Task*)(&req));
  }
  // end block
  for (int i = 0; i < NUM_DPU; i++) {
    BufferBuilderEndBlock(&builders[i]);
    buffers[i] = BufferBuilderFinish(&builders[i], &sizes[i]);
  }
  // get max sizes
  qsort(sizes, NUM_DPU, sizeof(size_t), cmpfunc);
  // send
  struct dpu_set_t set, dpu;

  DPU_ASSERT(dpu_alloc(NUM_DPU, NULL, &set));
  DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

  uint32_t idx;
  DPU_FOREACH(set, dpu, idx) {
    DPU_ASSERT(dpu_prepare_xfer(dpu, buffers[idx]));
  }
  DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "receiveBuffer", 0, sizes[0], DPU_XFER_DEFAULT));
  DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
  ReadDpuSetLog(set);
  //free
  DPU_ASSERT(dpu_free(set));
  for (int i = 0; i < NUM_DPU; i++) {
    free(buffers[i]);
  }  
}

void SendCreateIndexReq(HashTableId indexId){
  // prepare dpu buffers
  uint8_t *buffers[NUM_DPU];
  size_t sizes[NUM_DPU];
  BufferBuilder builders[NUM_DPU];
  CpuToDpuBufferDescriptor bufferDescs[NUM_DPU];
  for (int i = 0; i < NUM_DPU; i++) {
    bufferDescs[i] = (CpuToDpuBufferDescriptor) {
      .header = {
	.epochNumber = GetEpochNumber(),
      }
    };
  }
  for (int i = 0; i < NUM_DPU; i++) {
    BufferBuilderInit(&builders[i], &bufferDescs[i]);
    BufferBuilderBeginBlock(&builders[i], CREATE_INDEX_REQ);
  }
  for (int i = 0; i < NUM_DPU; i++){
    int dpuIdx = i;
    CreateIndexReq req;
    req.base = (Task) { .taskType = CREATE_INDEX_REQ };
    req.hashTableId = indexId;
    // append one task for each dpu
    BufferBuilderAppendTask(&builders[dpuIdx], (Task*)(&req));
  }
  // end block
  for (int i = 0; i < NUM_DPU; i++) {
    BufferBuilderEndBlock(&builders[i]);
    buffers[i] = BufferBuilderFinish(&builders[i], &sizes[i]);
  }
  // get max sizes
  qsort(sizes, NUM_DPU, sizeof(size_t), cmpfunc);
  // send
  struct dpu_set_t set, dpu;

  DPU_ASSERT(dpu_alloc(NUM_DPU, NULL, &set));
  DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

  uint32_t idx;
  DPU_FOREACH(set, dpu, idx) {
    DPU_ASSERT(dpu_prepare_xfer(dpu, buffers[idx]));
  }
  DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "receiveBuffer", 0, sizes[0], DPU_XFER_DEFAULT));
  DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
  //free
  DPU_ASSERT(dpu_free(set));
  for (int i = 0; i < NUM_DPU; i++) {
    free(buffers[i]);
  }  
}

void SendGetOrInsertReq(uint32_t tableId, Key *keys, uint64_t *tupleAddrs, size_t batchSize, uint8_t *recvBuffers[])
{
  ValidValueCheck(batchSize <= BATCH_SIZE * NUM_DPU);
  // prepare dpu buffers
  uint8_t *buffers[NUM_DPU];
  size_t sizes[NUM_DPU];
  BufferBuilder builders[NUM_DPU];
  CpuToDpuBufferDescriptor bufferDescs[NUM_DPU];
  for (int i = 0; i < NUM_DPU; i++) {
    bufferDescs[i] = (CpuToDpuBufferDescriptor) {
      .header = {
	.epochNumber = GetEpochNumber(),
      }
    };
  }
  for (int i = 0; i < NUM_DPU; i++) {
    BufferBuilderInit(&builders[i], &bufferDescs[i]);
    BufferBuilderBeginBlock(&builders[i], GET_OR_INSERT_REQ);
  }
  for (int i = 0; i < batchSize; i++){
    int dpuIdx = i % NUM_DPU;
    size_t taskSize = ROUND_UP_TO_8(keys[i].len) + sizeof(GetOrInsertReq);
    GetOrInsertReq *req = malloc(taskSize);
    memset(req, 0, taskSize);
    req->base = (Task) { .taskType = GET_OR_INSERT_REQ };
    req->len = keys[i].len;
    req->tid = (TupleIdT) { .tableId = tableId, .tupleAddr = tupleAddrs[i] };
    req->hashTableId = i % NUM_DPU;
    memcpy(req->ptr, keys[i].data, keys[i].len);
  // append one task for each dpu
    BufferBuilderAppendTask(&builders[dpuIdx], (Task*)req);
  }
  // end block
  for (int i = 0; i < NUM_DPU; i++) {
    BufferBuilderEndBlock(&builders[i]);
    buffers[i] = BufferBuilderFinish(&builders[i], &sizes[i]);
  }
  // get max sizes
  qsort(sizes, NUM_DPU, sizeof(size_t), cmpfunc);
  // send
  struct dpu_set_t set, dpu;

  DPU_ASSERT(dpu_alloc(NUM_DPU, NULL, &set));
  DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

  uint32_t idx;
  DPU_FOREACH(set, dpu, idx) {
    DPU_ASSERT(dpu_prepare_xfer(dpu, buffers[idx]));
  }
  DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "receiveBuffer", 0, sizes[0], DPU_XFER_DEFAULT));
  DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));

  // receive
  DPU_FOREACH(set, dpu, idx) {
    DPU_ASSERT(dpu_prepare_xfer(dpu, recvBuffers[idx]));
  }
  // how to get the reply buffer size? it seems that the reply buffer size will less then send buffer size
  DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, "replyBuffer", 0, sizes[0], DPU_XFER_DEFAULT));
  //free
  DPU_ASSERT(dpu_free(set));
  for (int i = 0; i < NUM_DPU; i++) {
    free(buffers[i]);
  }  
}
