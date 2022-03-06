"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
class Autocomplete extends HTMLElement {
    constructor() {
        super();
        this.inputDelay = 300;
        this.minQuery = 1;
        this.maxOptions = 10;
        this.api = '';
        this.input = null;
        this.datalist = null;
        this.handleInput = (e) => {
            clearTimeout(this.debounce);
            this.debounce = setTimeout(() => __awaiter(this, void 0, void 0, function* () {
                var _a, _b;
                const q = ((_a = this.input) === null || _a === void 0 ? void 0 : _a.value) || '';
                let completions;
                if (Object.keys(this.cache).indexOf(q) !== -1) {
                    completions = this.cache[q];
                }
                else {
                    completions = yield this.callApi(((_b = this.input) === null || _b === void 0 ? void 0 : _b.value) || '');
                    this.cache[q] = completions;
                }
                this.updateDatalist(completions);
            }), this.inputDelay);
        };
        this.cache = {};
        this.datalistId = 'so_libhack_wikidice__autocomplete_datalist_id';
    }
    static get observedAttributes() {
        return ['api', 'min-query', 'max-options'];
    }
    attributeChangedCallback(name, oldValue, newValue) {
        if (oldValue === newValue) {
            return;
        }
        switch (name) {
            case 'api':
                this.api = newValue;
                break;
            case 'min-query':
                this.minQuery = newValue;
                break;
            case 'max-options':
                this.maxOptions = newValue;
                break;
            default:
                console.warn(`No attribute named "${name}" exists on <lh-autocomplete/>`);
        }
    }
    connectedCallback() {
        const input = this.firstElementChild;
        if ((input === null || input === void 0 ? void 0 : input.nodeName) === 'INPUT') {
            this.input = input;
        }
        else {
            throw new Error(`Child element passed to <lh-autocomplete/> must be an <input>`);
        }
        const dl = document.createElement('datalist');
        dl.id = this.datalistId;
        this.datalist = this.insertBefore(dl, this.input);
        this.input.setAttribute('autocomplete', 'off');
        this.input.setAttribute('list', this.datalistId);
        this.input.addEventListener('input', this.handleInput);
    }
    disconnectedCallback() {
        var _a, _b, _c;
        (_a = this.input) === null || _a === void 0 ? void 0 : _a.removeEventListener('input', this.handleInput);
        (_b = this.input) === null || _b === void 0 ? void 0 : _b.removeAttribute('list');
        (_c = this.datalist) === null || _c === void 0 ? void 0 : _c.remove();
    }
    callApi(query) {
        return __awaiter(this, void 0, void 0, function* () {
            if (query.length < this.minQuery) {
                return [];
            }
            const r = yield fetch(`${this.api}${query}`);
            if (r.status !== 200) {
                console.error(`Failure to fetch autocomplete results: ${yield r.text()}`);
                return [];
            }
            let completions = yield r.json();
            return completions.slice(0, this.maxOptions);
        });
    }
    updateDatalist(completions) {
        var _a, _b;
        while ((_a = this.datalist) === null || _a === void 0 ? void 0 : _a.firstChild) {
            this.datalist.removeChild(this.datalist.firstChild);
        }
        for (const completion of completions) {
            const option = document.createElement('option');
            option.value = completion;
            (_b = this.datalist) === null || _b === void 0 ? void 0 : _b.appendChild(option);
        }
    }
}
window.customElements.define('lh-autocomplete', Autocomplete);
