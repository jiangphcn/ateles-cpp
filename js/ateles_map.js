let lib = {};
const mapFuns = new Map();
let results = [];

function emit(key, value) {
    results.push([key, value]);
}

function setLib(source) {
    lib = JSON.parse(source);
}

function addFun(id, source) {
    const fun = eval(source);
    if (typeof fun === "function") {
        mapFuns.set(id, fun);
    } else {
        throw "Invalid function";
    }
}

const mapFunRunner = (doc, mapFn) => {
    try {
        results = [];
        mapFn(doc);
        return results;
    } catch (ex) {
        return { error: ex.toString() };
    }
};

function mapDoc(doc_str) {
    const doc = JSON.parse(doc_str);
    const mapResults = Array.from(mapFuns, ([id, mapFun]) => {
        const mapResult = { id };

        const result = mapFunRunner(doc, mapFun);
        if (result.error) {
            mapResult.error = result.error;
        } else {
            mapResult.result = result;
        }

        return mapResult;
    });

    return JSON.stringify(mapResults);
}
