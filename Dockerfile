FROM alpine:3.19 AS builder

RUN apk add --no-cache build-base curl-dev taglib-dev

WORKDIR /build
COPY . .
RUN make clean && make

FROM alpine:3.19

RUN apk add --no-cache curl taglib-c libstdc++ libgcc

COPY --from=builder /build/synclyr2metadata /usr/local/bin/synclyr2metadata

ENTRYPOINT ["synclyr2metadata"]
